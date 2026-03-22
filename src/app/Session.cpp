#include "Session.h"
#include <thread>

Session::Session(int seed)
    : world(seed), pokedex(), ui(), saveManager(), story(), music(), sound(), cutsceneRunner(),
      ctx(world, pokedex, ui, saveManager, story, music, cutsceneRunner, sound), coordinator(ctx) {}

Session::~Session() = default;

// ── Main loop
// ──────────────────────────────────────────────────────────────────

void Session::run() {
    while (!ui.shouldClose()) {
        tick();
    }
}

void Session::tick() {
#ifndef __EMSCRIPTEN__
    auto frameStart = std::chrono::high_resolution_clock::now();
#endif

    ui.beginFrame();

    coordinator.update(ui.getInput());
    coordinator.processRequests();
    coordinator.render();

    ui.updateInput();
    ui.endFrame();

#ifndef __EMSCRIPTEN__
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double>(frameEnd - frameStart);
    if (elapsed < targetFrameTime)
        std::this_thread::sleep_for(targetFrameTime - elapsed);
#endif
}

// ── Initialisation
// ─────────────────────────────────────────────────────────────

void Session::init() {
    pokedex.loadSpecies("assets/data/species.txt");
    pokedex.loadMoves("assets/data/moves.txt");
    pokedex.loadItems("assets/data/items.txt");

    ui.loadBattleAssets();
    ui.getOverworldRenderer().loadSprites();
    world.generate(pokedex);
    ui.getOverworldRenderer().loadMapBackgrounds(world);
    music.loadAll();
    sound.loadAll();

    coordinator.switchMode(GameState::titleScreen);
    music.play(MusicTrack::titleScreen, ui.getRenderer().getWindow());
}

// ── Getters
// ────────────────────────────────────────────────────────────────────

World &Session::getWorld() { return world; }
Pokedex &Session::getPokedex() { return pokedex; }
GameUI &Session::getUI() { return ui; }
SaveManager &Session::getSaveManager() { return saveManager; }
