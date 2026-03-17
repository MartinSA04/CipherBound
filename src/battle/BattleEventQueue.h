#pragma once

#include "BattleTypes.h"
#include <deque>
#include <string>

class BattleEventQueue {
  public:
    enum class EventType {
        message,
        hpAnimation,
        expAnimation,
        introAnimation,
        captureAnimation,
        attackAnimationPlayer,
        attackAnimationOpponent,
    };

    void pushMessage(std::string message);
    void pushHPAnimation();
    void pushEXPAnimation();
    void pushIntroAnimation();
    void pushCaptureAnimation();
    void pushAttackAnimation(bool isPlayer);
    void pushLevelUpResume(std::string message);

    bool empty() const;
    const std::string *currentMessage() const;
    void popCurrentMessage();

    BattleState consume(BattleState pendingState, int &introPhase, bool &attackAnimIsPlayer);

  private:
    struct Event {
        EventType type;
        std::string text;
    };

    std::deque<Event> entries;
};
