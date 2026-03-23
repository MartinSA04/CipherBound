#include "BattleEventQueue.h"

void BattleEventQueue::pushMessage(std::string message) {
    entries.push_back({EventType::message, std::move(message)});
}

void BattleEventQueue::pushHPAnimation() { entries.push_back({EventType::hpAnimation, {}}); }

void BattleEventQueue::pushEXPAnimation() { entries.push_back({EventType::expAnimation, {}}); }

void BattleEventQueue::pushIntroAnimation() { entries.push_back({EventType::introAnimation, {}}); }

void BattleEventQueue::pushCaptureAnimation() {
    entries.push_back({EventType::captureAnimation, {}});
}

void BattleEventQueue::pushAttackAnimation(bool isPlayer) {
    entries.push_back(
        {isPlayer ? EventType::attackAnimationPlayer : EventType::attackAnimationOpponent, {}});
}

void BattleEventQueue::pushSwitchAnimation(bool isRecall) {
    entries.push_back(
        {isRecall ? EventType::switchAnimationRecall : EventType::switchAnimationSendOut, {}});
}

void BattleEventQueue::pushLevelUpResume(std::string message) {
    entries.push_front({EventType::expAnimation, {}});
    entries.push_front({EventType::message, std::move(message)});
}

bool BattleEventQueue::empty() const { return entries.empty(); }

const std::string *BattleEventQueue::currentMessage() const {
    if (!entries.empty() && entries.front().type == EventType::message)
        return &entries.front().text;
    return nullptr;
}

void BattleEventQueue::popCurrentMessage() {
    if (!entries.empty() && entries.front().type == EventType::message)
        entries.pop_front();
}

BattleState BattleEventQueue::consume(BattleState pendingState, int &introPhase,
                                      bool &attackAnimIsPlayer, bool &switchAnimIsRecall) {
    if (entries.empty())
        return pendingState;

    const Event &entry = entries.front();
    switch (entry.type) {
    case EventType::message:
        return BattleState::showingMessages;
    case EventType::hpAnimation:
        entries.pop_front();
        return BattleState::animatingHP;
    case EventType::expAnimation:
        entries.pop_front();
        return BattleState::animatingEXP;
    case EventType::introAnimation:
        entries.pop_front();
        introPhase++;
        return BattleState::intro;
    case EventType::captureAnimation:
        entries.pop_front();
        return BattleState::animatingCapture;
    case EventType::attackAnimationPlayer:
        entries.pop_front();
        attackAnimIsPlayer = true;
        return BattleState::animatingAttack;
    case EventType::attackAnimationOpponent:
        entries.pop_front();
        attackAnimIsPlayer = false;
        return BattleState::animatingAttack;
    case EventType::switchAnimationRecall:
        entries.pop_front();
        switchAnimIsRecall = true;
        return BattleState::animatingSwitch;
    case EventType::switchAnimationSendOut:
        entries.pop_front();
        switchAnimIsRecall = false;
        return BattleState::animatingSwitch;
    }

    return pendingState;
}
