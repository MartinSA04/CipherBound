#include "../../src/cutscene/CutscenePlayback.h"
#include <cassert>

int main() {
    CutscenePlaybackState playback;
    assert(playback.isFinished());

    playback.reset();
    assert(!playback.isFinished());
    assert(playback.stepIndex() == 0);
    assert(playback.pendingMoves().empty());

    playback.queueMove("player", {2, 3});
    assert(playback.pendingMoves().size() == 1);

    playback.beginDialogue();
    assert(playback.isShowingDialogue());
    playback.endDialogue();
    assert(!playback.isShowingDialogue());

    playback.beginWait(2);
    assert(playback.isWaiting());
    assert(!playback.tickWait());
    assert(playback.isWaiting());
    assert(playback.tickWait());
    assert(!playback.isWaiting());

    playback.advanceStep();
    assert(playback.stepIndex() == 1);

    playback.clearPendingMoves();
    assert(playback.pendingMoves().empty());

    playback.finish();
    assert(playback.isFinished());
}
