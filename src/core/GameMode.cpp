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

void ModeMailbox::push(ModeRequest req) { pending.push_back(std::move(req)); }

std::vector<ModeRequest> ModeMailbox::drain() {
    std::vector<ModeRequest> requests;
    std::swap(requests, pending);
    return requests;
}

void GameContext::setBattle(std::unique_ptr<Battle> battle) {
    battleSession.active = std::move(battle);
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

void GameContext::clearBattle() { battleSession.active.reset(); }

void GameContext::pushRequest(ModeRequest req) { mailbox.push(std::move(req)); }

void GameContext::playSound(SoundEffect sfx) { sound.play(sfx, ui.getRenderer().getWindow()); }

// ── ModeRequest factory helpers
// ────────────────────────────────────────────────

ModeRequest ModeRequest::changeState(GameState s) {
    ModeRequest r{};
    r.type = Type::changeState;
    r.targetState = s;
    return r;
}

ModeRequest ModeRequest::wildBattle(int species, int level) {
    ModeRequest r{};
    r.type = Type::startWildBattle;
    r.speciesId = species;
    r.level = level;
    return r;
}

ModeRequest ModeRequest::trainerBattle(NPC *t) {
    ModeRequest r{};
    r.type = Type::startTrainerBattle;
    r.npc = t;
    return r;
}

ModeRequest ModeRequest::endBattle() {
    ModeRequest r{};
    r.type = Type::endBattle;
    return r;
}

ModeRequest ModeRequest::transition(const std::string &mapId, const Position &spawn) {
    ModeRequest r{};
    r.type = Type::transitionToMap;
    r.mapId = mapId;
    r.spawn = spawn;
    return r;
}

ModeRequest ModeRequest::dialogue(const std::string &speaker, const std::vector<std::string> &lines,
                                  NPC *npc, GameState retState) {
    ModeRequest r{};
    r.type = Type::startDialogue;
    r.speaker = speaker;
    r.lines = lines;
    r.npc = npc;
    r.returnState = retState;
    return r;
}

ModeRequest ModeRequest::dialogueChoice(const std::vector<std::string> &options,
                                        const std::string &context, GameState retState) {
    ModeRequest r{};
    r.type = Type::startDialogueChoice;
    r.choiceOptions = options;
    r.choiceContext = context;
    r.returnState = retState;
    return r;
}

ModeRequest ModeRequest::cutscene(const std::string &path) {
    ModeRequest r{};
    r.type = Type::startCutscene;
    r.cutscenePath = path;
    return r;
}

ModeRequest ModeRequest::storyAction(const StoryAction &action) {
    ModeRequest r{};
    r.type = Type::handleStoryAction;
    r.storyActionData = action;
    return r;
}

// ── GameMode helpers
// ───────────────────────────────────────────────────────────

void GameMode::renderOverworld(GameContext &ctx) {
    const std::string &mapId = ctx.world.getCurrentMapId();
    ctx.ui.drawOverworld(ctx.world.getMap(mapId), ctx.world.getPlayer(), ctx.world.getNPCs(mapId));
}
