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

class Session {
  public:
    explicit Session(int seed);
    ~Session();

    // Initialise game data and subsystems (call before run/tick)
    void init();

    // Main game loop (native only — blocks until window closes)
    void run();

    // Execute a single frame (used by Emscripten main loop callback)
    void tick();

    // Access subsystems
    World &getWorld();
    Pokedex &getPokedex();
    GameUI &getUI();
    SaveManager &getSaveManager();

  private:
    // Subsystems
    World world;
    Pokedex pokedex;
    GameUI ui;
    SaveManager saveManager;
    StoryManager story;
    MusicManager music;
    SoundManager sound;
    CutsceneRunner cutsceneRunner;

    // Shared context (references into subsystems above)
    GameContext ctx;
    SessionCoordinator coordinator;

    static constexpr int targetFPS = 60;
    static constexpr std::chrono::duration<double> targetFrameTime{1.0 / targetFPS};
};
