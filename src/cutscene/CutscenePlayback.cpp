#include "CutscenePlayback.h"

void CutscenePlaybackState::reset() {
    currentStep = 0;
    finished = false;
    moves.clear();
    waitFrames = 0;
    inDialogue = false;
}

bool CutscenePlaybackState::isFinished() const { return finished; }

bool CutscenePlaybackState::isShowingDialogue() const { return inDialogue; }

bool CutscenePlaybackState::isWaiting() const { return waitFrames > 0; }

std::size_t CutscenePlaybackState::stepIndex() const { return currentStep; }

void CutscenePlaybackState::advanceStep() { ++currentStep; }

void CutscenePlaybackState::finish() { finished = true; }

void CutscenePlaybackState::beginDialogue() { inDialogue = true; }

void CutscenePlaybackState::endDialogue() { inDialogue = false; }

void CutscenePlaybackState::beginWait(int frames) { waitFrames = (frames > 0) ? frames : 0; }

bool CutscenePlaybackState::tickWait() {
    if (waitFrames <= 0)
        return false;

    --waitFrames;
    return waitFrames <= 0;
}

void CutscenePlaybackState::queueMove(std::string targetId, Position destination) {
    moves.push_back(CutscenePendingMove{std::move(targetId), destination});
}

void CutscenePlaybackState::clearPendingMoves() { moves.clear(); }

std::vector<CutscenePendingMove> &CutscenePlaybackState::pendingMoves() { return moves; }

const std::vector<CutscenePendingMove> &CutscenePlaybackState::pendingMoves() const {
    return moves;
}
