/**
 * @file
 * @brief Top-level game session bootstrap and frame loop.
 * @ingroup app_core
 */

#pragma once
#include "../audio/MusicManager.h"
#include "../audio/SoundManager.h"
#include "../cutscene/CutsceneRunner.h"
#include "../game_data/Pokedex.h"
#include "../save/SaveManager.h"
#include "../state/World.h"
#include "../story/StoryManager.h"
#include "../ui/GameUI.h"
#include "GameMode.h"
#include "SessionCoordinator.h"
#include <chrono>

/**
 * @brief Owns all long-lived subsystems for one running game session.
 * @ingroup app_core
 */
class Session {
  public:
    /// Creates a session with a deterministic world seed.
    explicit Session(int seed);
    /// Destroys the session and owned subsystems.
    ~Session();

    /// Loads game data, assets, and initial mode state.
    void init();

    /// Runs the native main loop until the window closes.
    void run();

    /// Executes one frame of update, request processing, and rendering.
    void tick();

    /// Returns the owned world model.
    World &getWorld();
    /// Returns the loaded game-data catalog.
    Pokedex &getPokedex();
    /// Returns the owned UI facade.
    GameUI &getUI();
    /// Returns the save manager.
    SaveManager &getSaveManager();

  private:
    World world;                   ///< Owned world model.
    Pokedex pokedex;               ///< Loaded game-data tables.
    GameUI ui;                     ///< Owned UI facade.
    SaveManager saveManager;       ///< Save/load orchestration.
    StoryManager story;            ///< Story progression logic.
    MusicManager music;            ///< Background music playback.
    SoundManager sound;            ///< Sound effect playback.
    CutsceneRunner cutsceneRunner; ///< Scripted cutscene runner.

    GameContext ctx;                ///< Shared mode context referencing owned subsystems.
    SessionCoordinator coordinator; ///< Active mode coordinator.

    static constexpr int targetFPS = 60;
    static constexpr std::chrono::duration<double> targetFrameTime{1.0 / targetFPS};
};
