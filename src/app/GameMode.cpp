#include "GameMode.h"
#include "../audio/MusicManager.h"
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

BattlePresentationState &GameContext::battlePresentation() { return battleSession.presentation; }

const BattlePresentationState &GameContext::battlePresentation() const {
    return battleSession.presentation;
}

void GameContext::resetBattlePresentation() { battleSession.presentation.reset(); }

void GameContext::clearBattle() {
    battleSession.active.reset();
    battleSession.trainerNPCId.clear();
    battleSession.presentation.reset();
}

void GameContext::pushRequest(ModeRequest req) { mailbox.push(std::move(req)); }

void GameContext::playSound(SoundEffect sfx) { sound.play(sfx, ui.getRenderer().getWindow()); }

void GameContext::playMusic(MusicTrack track) { music.play(track, ui.getRenderer().getWindow()); }

void GameContext::playMusicOneShot(MusicTrack track) {
    music.playOneShot(track, ui.getRenderer().getWindow());
}

void GameContext::playCurrentMapMusic() {
    music.playPath(world.getMap(world.getCurrentMapId()).getMusicPath(),
                   ui.getRenderer().getWindow());
}

void GameContext::stopMusic() { music.stop(); }

// ── GameMode helpers
// ───────────────────────────────────────────────────────────

void GameMode::renderOverworld(GameContext &ctx) {
    const std::string &mapId = ctx.world.getCurrentMapId();
    ctx.ui.drawOverworld(ctx.world.getMap(mapId), ctx.world.getPlayer(), ctx.world.getNPCs(mapId));
}
