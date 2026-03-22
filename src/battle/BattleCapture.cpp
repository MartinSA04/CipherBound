#include "BattleCapture.h"
#include "../game_data/Item.h"
#include "../state/Daemon.h"
#include <algorithm>
#include <cmath>

namespace {

constexpr int guaranteedCaptureThreshold = 255;
constexpr int captureShakeRollMax = 65535;
constexpr int guaranteedCaptureShakes = 4;

float statusBonusFor(const Daemon &target) {
    if (target.getStatus() == StatusEffect::deadlocked ||
        target.getStatus() == StatusEffect::entangled)
        return 2.0f;
    if (target.getStatus() != StatusEffect::none)
        return 1.5f;
    return 1.0f;
}

} // namespace

BattleCaptureOutcome BattleCapture::resolve(const Daemon &target, const ItemData &ball,
                                            std::mt19937 &rng) {
    const Species &species = target.getSpecies();
    const int maxHP = target.getMaxHP();
    const int currentHP = target.getCurrentHP();
    const int catchRate = species.catchRate;
    const int ballModifier = ball.effectValue;

    float a = ((3.0f * static_cast<float>(maxHP) - 2.0f * static_cast<float>(currentHP)) *
               static_cast<float>(catchRate) * static_cast<float>(ballModifier)) /
              (3.0f * static_cast<float>(maxHP)) * statusBonusFor(target);
    a = std::min(a, static_cast<float>(guaranteedCaptureThreshold));

    if (a >= static_cast<float>(guaranteedCaptureThreshold))
        return BattleCaptureOutcome{true, guaranteedCaptureShakes};

    const float b = 1048560.0f / std::sqrt(std::sqrt(16711680.0f / a));
    const int shakeThreshold = static_cast<int>(b);

    std::uniform_int_distribution<int> roll(0, captureShakeRollMax);
    int shakes = 0;
    for (int i = 0; i < guaranteedCaptureShakes; ++i) {
        if (roll(rng) < shakeThreshold)
            shakes++;
        else
            break;
    }

    return BattleCaptureOutcome{shakes == guaranteedCaptureShakes, shakes};
}

Daemon BattleCapture::caughtDaemon(const Daemon &target) {
    return Daemon(target.getSpecies(), target.getLevel(), target.getExp(), target.getCurrentHP(),
                  target.getNickname(), target.getStatus(), target.getIVs(), target.getEVs(),
                  target.getMoves(), target.getNature());
}

std::string_view BattleCapture::failureMessage(int shakes) {
    switch (shakes) {
    case 0:
        return "Oh no! The Daemon broke free immediately!";
    case 1:
        return "The ball shook once... but it broke free!";
    case 2:
        return "The ball shook twice... but it broke free!";
    default:
        return "The ball shook three times... So close!";
    }
}
