/**
 * @file
 * @brief High-level cutscene executor used by the app flow.
 * @ingroup cutscene_system
 */

#pragma once
#include "CutscenePlayback.h"
#include "../game_data/Cutscene.h"
#include <string>

class World;
class GameUI;
class SoundManager;

/**
 * @brief Loads, starts, and advances cutscenes against the live world and UI.
 * @ingroup cutscene_system
 */
class CutsceneRunner {
  public:
    /// Creates an idle cutscene runner with no loaded cutscene.
    CutsceneRunner() = default;

    /// Loads a cutscene from a `.cutscene` file.
    bool load(const std::string &path);
    /// Loads a cutscene value directly.
    bool load(Cutscene cutscene);

    /// Starts playback from the beginning and resets transient state.
    void start();

    /// Advances playback by one frame and returns whether it is still running.
    bool update(World &world, GameUI &ui, bool confirmPressed, SoundManager &sound);

    /// Returns whether playback has completed.
    bool isFinished() const;

    /// Returns whether cutscene dialogue is currently visible.
    bool isShowingDialogue() const;

    /// Returns the loaded cutscene id.
    const std::string &getId() const;

  private:
    Cutscene cutscene;
    CutscenePlaybackState playback;

    // Process steps from currentStep onward until hitting a blocking step
    void processSteps(World &world, GameUI &ui);
};
