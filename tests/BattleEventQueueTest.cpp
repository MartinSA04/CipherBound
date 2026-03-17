#include "../src/battle/BattleEventQueue.h"
#include <cassert>

int main() {
    BattleEventQueue queue;
    int introPhase = 0;
    bool playerAttack = false;

    queue.pushMessage("First");
    assert(queue.currentMessage() != nullptr);
    assert(*queue.currentMessage() == "First");
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack) ==
           BattleState::showingMessages);

    queue.popCurrentMessage();
    queue.pushHPAnimation();
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack) ==
           BattleState::animatingHP);

    queue.pushIntroAnimation();
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack) ==
           BattleState::intro);
    assert(introPhase == 1);

    queue.pushAttackAnimation(false);
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack) ==
           BattleState::animatingAttack);
    assert(!playerAttack);

    assert(queue.consume(BattleState::victory, introPhase, playerAttack) == BattleState::victory);
    return 0;
}
