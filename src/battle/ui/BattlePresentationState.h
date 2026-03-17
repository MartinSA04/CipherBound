#pragma once

enum class EXPTickResult {
    inProgress,
    reachedTarget,
    filledBar,
};

struct BattlePresentationState {
    static constexpr int introTransitionDuration = 90;
    static constexpr int introSceneDuration = 46;

    int playerDisplayHP{0};
    int opponentDisplayHP{0};
    int playerDisplayEXP{0};
    int expAnimFrame{0};
    int expAnimStartEXP{-1};

    int introFrame{0};
    int introPhase{0};

    void reset();
    void beginBattle(int playerHP, int opponentHP, int playerEXP);
    void resetExpAnimation();

    bool tickHPAnimation(int targetPlayerHP, int targetOpponentHP, int maxPlayerHP,
                         int maxOpponentHP);
    EXPTickResult tickEXPAnimation(int targetEXP, int expNeeded);
};
