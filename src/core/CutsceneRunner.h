#pragma once
#include "../audio/SoundManager.h"
#include "../data/Cutscene.h"
#include "../state/World.h"
#include "../ui/GameUI.h"
#include <cstddef>
#include <string>
#include <vector>

class CutsceneRunner {
  public:
    CutsceneRunner() = default;

    // Load a cutscene from a .cutscene file
    bool load(const std::string &path);

    // Start playback (resets state)
    void start();

    // Update one frame. Returns true while the cutscene is still running.
    // confirmPressed: whether the player pressed confirm this frame (for
    // dialogue).
    bool update(World &world, GameUI &ui, bool confirmPressed, SoundManager &sound);

    // True if the cutscene has finished
    bool isFinished() const;

    // True if a dialogue box is currently showing
    bool isShowingDialogue() const;

    const std::string &getId() const;

  private:
    Cutscene cutscene;
    std::size_t currentStep{0};
    bool finished{true};

    // --- Movement tracking ---
    struct PendingMove {
        std::string targetId; // "player" or NPC id
        Position destination;
    };
    std::vector<PendingMove> pendingMoves;

    // --- Wait tracking ---
    int waitFrames{0};

    // --- Dialogue tracking ---
    bool inDialogue{false};

    // Process steps from currentStep onward until hitting a blocking step
    void processSteps(World &world, GameUI &ui);

    // Tick all pending movements one step closer to their targets
    void tickMovements(World &world);

    // Check if all pending movements have arrived
    bool allMovesComplete(const World &world) const;

    // Get current position of a target entity
    Position getEntityPosition(const World &world, const std::string &targetId) const;

    // Check if an entity is currently mid-walk-animation
    bool isEntityWalking(const World &world, const std::string &targetId) const;

    // Start a single tile step for an entity toward its destination
    void stepEntityToward(World &world, const std::string &targetId, Position dest);
};
