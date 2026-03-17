#include "../Battle.h"
#include "BattleMode.h"
#include <algorithm>

namespace {
constexpr int CAPTURE_THROW_FRAMES = 30;
constexpr int CAPTURE_LAND_FRAMES = 10;
constexpr int CAPTURE_SHAKE_FRAMES = 24;
constexpr int CAPTURE_SHAKE_PAUSE = 16;
} // namespace

void BattleMode::updateCaptureAnim(GameContext &ctx) {
    Battle &battle = ctx.battle();
    captureAnimFrame++;

    int totalShakes = battle.getCaptureShakes();
    int throwEnd = CAPTURE_THROW_FRAMES;
    int landEnd = throwEnd + CAPTURE_LAND_FRAMES;

    int shakeGroupLen = CAPTURE_SHAKE_FRAMES + CAPTURE_SHAKE_PAUSE;

    if (captureAnimFrame >= landEnd) {
        int shakeFrame = captureAnimFrame - landEnd;
        int currentShake = shakeFrame / shakeGroupLen;
        int frameInShake = shakeFrame % shakeGroupLen;

        if (currentShake < totalShakes && currentShake < 4) {
            if (frameInShake == 0)
                ctx.playSound(SoundEffect::pokeballShake);
        }
    }

    int shakesEnd = landEnd + std::min(totalShakes, 4) * shakeGroupLen;
    int totalEnd = shakesEnd + 20;

    if (captureAnimFrame >= totalEnd) {
        if (!battle.getCaptureSuccess())
            ctx.playSound(SoundEffect::pokeballEscape);

        captureAnimFrame = 0;
        captureAnimDone = true;
        battle.finishCaptureAnimation();
    }
}
