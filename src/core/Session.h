#pragma once
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include "GameMode.h"
#include "../world/World.h"
#include "../data/Pokedex.h"
#include "../save/SaveManager.h"
#include "../ui/GameUI.h"
#include "StoryManager.h"
#include "CutsceneRunner.h"
#include "../audio/MusicManager.h"
#include "../audio/SoundManager.h"

class Session
{
public:
    explicit Session(unsigned long seed);

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
    unsigned long seed;

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

    // Current active mode and state
    std::unique_ptr<GameMode> currentMode;
    GameState currentState{GameState::titleScreen};

    // Process pending requests from the active mode
    void processRequests();

    // Handle a StoryAction result (cross-mode orchestration)
    void handleStoryAction(const StoryAction &action);

    // Switch to a new mode for the given state
    void switchMode(GameState newState);

    // Switch to a pre-created mode for the given state
    void switchToMode(GameState newState, std::unique_ptr<GameMode> mode);

    // Create the appropriate GameMode object for a state
    std::unique_ptr<GameMode> createMode(GameState state);

    // Dialogue return state tracking (shared across dialogue/choice modes)
    GameState dialogueReturnState{GameState::overworld};

    // Maps GameState → ScreenType for the UI
    static ScreenType screenForState(GameState gs);

    static constexpr int targetFPS = 60;
    static constexpr std::chrono::duration<double> targetFrameTime{1.0 / targetFPS};
};
