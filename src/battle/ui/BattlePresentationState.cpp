#include "BattlePresentationState.h"
#include "BattleAnimationUtils.h"
#include <algorithm>

void BattlePresentationState::reset() {
    playerDisplayHP = 0;
    opponentDisplayHP = 0;
    playerDisplayEXP = 0;
    introFrame = 0;
    introPhase = 0;
    resetExpAnimation();
}

void BattlePresentationState::beginBattle(int playerHP, int opponentHP, int playerEXP) {
    playerDisplayHP = playerHP;
    opponentDisplayHP = opponentHP;
    playerDisplayEXP = playerEXP;
    introFrame = 0;
    introPhase = 0;
    resetExpAnimation();
}

void BattlePresentationState::resetExpAnimation() {
    expAnimFrame = 0;
    expAnimStartEXP = -1;
}

bool BattlePresentationState::tickHPAnimation(int targetPlayerHP, int targetOpponentHP,
                                              int maxPlayerHP, int maxOpponentHP) {
    const int playerStep = std::max(1, maxPlayerHP / 15);
    const int opponentStep = std::max(1, maxOpponentHP / 15);

    if (playerDisplayHP > targetPlayerHP)
        playerDisplayHP = std::max(targetPlayerHP, playerDisplayHP - playerStep);
    else if (playerDisplayHP < targetPlayerHP)
        playerDisplayHP = std::min(targetPlayerHP, playerDisplayHP + playerStep);

    if (opponentDisplayHP > targetOpponentHP)
        opponentDisplayHP = std::max(targetOpponentHP, opponentDisplayHP - opponentStep);
    else if (opponentDisplayHP < targetOpponentHP)
        opponentDisplayHP = std::min(targetOpponentHP, opponentDisplayHP + opponentStep);

    return playerDisplayHP == targetPlayerHP && opponentDisplayHP == targetOpponentHP;
}

EXPTickResult BattlePresentationState::tickEXPAnimation(int targetEXP, int expNeeded) {
    static constexpr int expAnimFrames = 120;
    const int destination = std::min(targetEXP, expNeeded);

    if (expAnimStartEXP < 0)
        expAnimStartEXP = playerDisplayEXP;

    expAnimFrame++;

    if (expAnimFrame >= expAnimFrames) {
        playerDisplayEXP = destination;
        resetExpAnimation();

        if (playerDisplayEXP >= expNeeded)
            return EXPTickResult::filledBar;
        return EXPTickResult::reachedTarget;
    }

    playerDisplayEXP = BattleAnimationUtils::interpolateLinearInt(expAnimStartEXP, destination,
                                                                  expAnimFrame, expAnimFrames);

    if (playerDisplayEXP >= expNeeded) {
        playerDisplayEXP = expNeeded;
        resetExpAnimation();
        return EXPTickResult::filledBar;
    }

    return EXPTickResult::inProgress;
}
