#pragma once

enum class BattleState {
    intro,
    choosingAction,
    choosingMove,
    choosingItem,
    choosingSwitch,
    showingMessages,
    animatingHP,
    animatingEXP,
    animatingCapture,
    animatingAttack,
    opponentTurn,
    victory,
    defeat,
    fled,
    captured,
};

enum class BattleAction {
    fight,
    item,
    switchDaemon,
    flee,
};

struct BattleResult {
    bool playerWon;
    bool playerFled;
    bool captured;
    int expGained;
    int moneyGained;
};

enum class BattleType {
    wild,
    trainer,
    gymLeader,
};
