#pragma once
#include "CutscenePlayback.h"
#include "../game_data/Cutscene.h"
#include <string>

class World;
class GameUI;
class SoundManager;

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
    CutscenePlaybackState playback;

    // Process steps from currentStep onward until hitting a blocking step
    void processSteps(World &world, GameUI &ui);
};
