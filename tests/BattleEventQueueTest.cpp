#include "../src/battle/BattleEventQueue.h"
#include <cassert>

int main() {
    BattleEventQueue queue;
    int introPhase = 0;
    bool playerAttack = false;
    bool switchRecall = false;
    bool switchPlayerSide = false;

    queue.pushMessage("First");
    assert(queue.currentMessage() != nullptr);
    assert(*queue.currentMessage() == "First");
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack, switchRecall,
                         switchPlayerSide) ==
           BattleState::showingMessages);

    queue.popCurrentMessage();
    queue.pushHPAnimation();
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack, switchRecall,
                         switchPlayerSide) ==
           BattleState::animatingHP);

    queue.pushIntroAnimation();
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack, switchRecall,
                         switchPlayerSide) ==
           BattleState::intro);
    assert(introPhase == 1);

    queue.pushAttackAnimation(false);
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack, switchRecall,
                         switchPlayerSide) ==
           BattleState::animatingAttack);
    assert(!playerAttack);

    queue.pushSwitchAnimation(true, false);
    assert(queue.consume(BattleState::choosingAction, introPhase, playerAttack, switchRecall,
                         switchPlayerSide) ==
           BattleState::animatingSwitch);
    assert(switchRecall);
    assert(!switchPlayerSide);

    assert(queue.consume(BattleState::victory, introPhase, playerAttack, switchRecall,
                         switchPlayerSide) ==
           BattleState::victory);
    return 0;
}
