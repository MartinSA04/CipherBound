#pragma once

#include "../state/Movement.h"
#include <cstddef>
#include <string>
#include <vector>

struct CutscenePendingMove {
    std::string targetId;
    Position destination;
};

class CutscenePlaybackState {
  public:
    void reset();

    bool isFinished() const;
    bool isShowingDialogue() const;
    bool isWaiting() const;
    std::size_t stepIndex() const;

    void advanceStep();
    void finish();

    void beginDialogue();
    void endDialogue();

    void beginWait(int frames);
    bool tickWait();

    void queueMove(std::string targetId, Position destination);
    void clearPendingMoves();

    std::vector<CutscenePendingMove> &pendingMoves();
    const std::vector<CutscenePendingMove> &pendingMoves() const;

  private:
    std::size_t currentStep{0};
    bool finished{true};
    std::vector<CutscenePendingMove> moves;
    int waitFrames{0};
    bool inDialogue{false};
};
