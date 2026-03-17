#include "GameMode.h"
#include "../audio/SoundManager.h"
#include "../battle/Battle.h"
#include "../state/World.h"
#include "../ui/GameUI.h"
#include <stdexcept>

// ── GameContext
// ────────────────────────────────────────────────────────────────

GameContext::GameContext(World &world, Pokedex &pokedex, GameUI &ui, SaveManager &saveManager,
                         StoryManager &story, MusicManager &music, CutsceneRunner &cutsceneRunner,
                         SoundManager &sound)
    : world(world), pokedex(pokedex), ui(ui), saveManager(saveManager), story(story), music(music),
      cutsceneRunner(cutsceneRunner), sound(sound) {}

GameContext::~GameContext() = default;

void GameContext::setBattle(std::unique_ptr<Battle> battle, std::string trainerNPCId) {
    battleSession.active = std::move(battle);
    battleSession.trainerNPCId = std::move(trainerNPCId);
}

bool GameContext::hasBattle() const { return static_cast<bool>(battleSession.active); }

Battle *GameContext::tryBattle() { return battleSession.active.get(); }

const Battle *GameContext::tryBattle() const { return battleSession.active.get(); }

Battle &GameContext::battle() {
    if (!battleSession.active)
        throw std::logic_error("No active battle in GameContext");
    return *battleSession.active;
}

const Battle &GameContext::battle() const {
    if (!battleSession.active)
        throw std::logic_error("No active battle in GameContext");
    return *battleSession.active;
}

const std::string &GameContext::battleTrainerNPCId() const { return battleSession.trainerNPCId; }

void GameContext::clearBattle() {
    battleSession.active.reset();
    battleSession.trainerNPCId.clear();
}

void GameContext::pushRequest(ModeRequest req) { mailbox.push(std::move(req)); }

void GameContext::playSound(SoundEffect sfx) { sound.play(sfx, ui.getRenderer().getWindow()); }

// ── GameMode helpers
// ───────────────────────────────────────────────────────────

void GameMode::renderOverworld(GameContext &ctx) {
    const std::string &mapId = ctx.world.getCurrentMapId();
    ctx.ui.drawOverworld(ctx.world.getMap(mapId), ctx.world.getPlayer(), ctx.world.getNPCs(mapId));
}
