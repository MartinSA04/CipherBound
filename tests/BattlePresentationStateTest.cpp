#include "../src/battle/ui/BattlePresentationState.h"
#include <cassert>

int main() {
    BattlePresentationState state;
    state.beginBattle(90, 75, 10);

    assert(state.playerDisplayHP == 90);
    assert(state.opponentDisplayHP == 75);
    assert(state.playerDisplayEXP == 10);
    assert(state.introFrame == 0);
    assert(state.introPhase == 0);
    assert(state.switchFrame == 0);
    assert(state.playerFieldVisible);

    const bool firstHpTick = state.tickHPAnimation(60, 45, 90, 75);
    assert(!firstHpTick);
    assert(state.playerDisplayHP < 90);
    assert(state.playerDisplayHP > 60);
    assert(state.opponentDisplayHP < 75);
    assert(state.opponentDisplayHP > 45);

    BattlePresentationState expState;
    expState.playerDisplayEXP = 0;
    EXPTickResult expResult = EXPTickResult::inProgress;
    for (int frame = 0; frame < 119; ++frame) {
        expResult = expState.tickEXPAnimation(120, 120);
        assert(expResult == EXPTickResult::inProgress);
    }

    expResult = expState.tickEXPAnimation(120, 120);
    assert(expResult == EXPTickResult::filledBar);
    assert(expState.playerDisplayEXP == 120);
    assert(expState.expAnimFrame == 0);
    assert(expState.expAnimStartEXP == -1);

    BattlePresentationState switchState;
    bool switchDone = false;
    for (int frame = 0; frame < BattlePresentationState::switchSceneDuration - 1; ++frame) {
        switchDone = switchState.tickSwitchAnimation();
        assert(!switchDone);
    }
    switchDone = switchState.tickSwitchAnimation();
    assert(switchDone);
    assert(switchState.switchFrame == 0);

    expState.reset();
    assert(expState.playerDisplayHP == 0);
    assert(expState.opponentDisplayHP == 0);
    assert(expState.playerDisplayEXP == 0);
    assert(expState.introFrame == 0);
    assert(expState.introPhase == 0);
    assert(expState.switchFrame == 0);
    assert(expState.playerFieldVisible);
}
