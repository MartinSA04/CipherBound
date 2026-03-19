/**
 * @file
 * @brief Mutable playback state for a running cutscene.
 * @ingroup cutscene_system
 */

#pragma once

#include "../state/Movement.h"
#include <cstddef>
#include <string>
#include <vector>

/// Queued movement command waiting for the world to finish animating.
struct CutscenePendingMove {
    std::string targetId;   ///< Target entity id, or `player`.
    Position destination;   ///< Destination tile for the queued move.
};

/**
 * @brief Tracks transient state while a cutscene is being executed frame-by-frame.
 * @ingroup cutscene_system
 */
class CutscenePlaybackState {
  public:
    /// Resets playback to the initial finished state.
    void reset();

    /// Returns whether the cutscene has finished.
    bool isFinished() const;
    /// Returns whether dialogue is currently being shown.
    bool isShowingDialogue() const;
    /// Returns whether playback is currently blocked on a wait command.
    bool isWaiting() const;
    /// Returns the index of the next step to process.
    std::size_t stepIndex() const;

    /// Advances to the next cutscene step.
    void advanceStep();
    /// Marks the cutscene as finished.
    void finish();

    /// Marks dialogue as active.
    void beginDialogue();
    /// Marks dialogue as complete.
    void endDialogue();

    /// Starts a blocking wait for the given number of frames.
    void beginWait(int frames);
    /// Advances the active wait and returns whether it should continue.
    bool tickWait();

    /// Queues a non-blocking movement for later world processing.
    void queueMove(std::string targetId, Position destination);
    /// Clears all queued moves.
    void clearPendingMoves();

    /// Returns mutable access to queued moves.
    std::vector<CutscenePendingMove> &pendingMoves();
    /// Returns immutable access to queued moves.
    const std::vector<CutscenePendingMove> &pendingMoves() const;

  private:
    std::size_t currentStep{0};
    bool finished{true};
    std::vector<CutscenePendingMove> moves;
    int waitFrames{0};
    bool inDialogue{false};
};
