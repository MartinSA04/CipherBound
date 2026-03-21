#include "../src/battle/BattleEventQueue.h"
#include <cassert>

int main() {
    BattleEventQueue queue;
    int introPhase = 0;
    bool playerAttack = false;
    bool switchRecall = false;

    queue.pushMessage("First");
    assert(queue.currentMessage() != nullptr);
    assert(*queue.currentMessage() == "First");
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack, switchRecall) ==
           BattleState::showingMessages);

    queue.popCurrentMessage();
    queue.pushHPAnimation();
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack, switchRecall) ==
           BattleState::animatingHP);

    queue.pushIntroAnimation();
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack, switchRecall) ==
           BattleState::intro);
    assert(introPhase == 1);

    queue.pushAttackAnimation(false);
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack, switchRecall) ==
           BattleState::animatingAttack);
    assert(!playerAttack);

    queue.pushSwitchAnimation(true);
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack, switchRecall) ==
           BattleState::animatingSwitch);
    assert(switchRecall);

    assert(queue.consume(BattleState::victory, introPhase, playerAttack, switchRecall) ==
           BattleState::victory);
    return 0;
}
