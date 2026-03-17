#pragma once
#include "../audio/SoundEffects.h"
#include "../data/Pokedex.h"
#include "../state/World.h"
#include "StoryAction.h"
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// Forward declarations for types not included through StoryManager.h
class NPC;
class MusicManager;
class CutsceneRunner;
class Battle;
class SaveManager;
class GameUI;
class InputManager;
class SoundManager;
class StoryManager;

// ── Game states
// ────────────────────────────────────────────────────────────────

enum class GameState {
    titleScreen,
    overworld,
    battle,
    battleIntro,
    menu,
    party,
    bag,
    saving,
    dialogue,
    dialogueChoice,
    transition,
    pcBox,
    cutscene,
    daemondex,
};

// ── Mode request (modes push these to trigger cross-mode transitions)
// ──────────

struct ChangeStateRequest {
    GameState targetState{GameState::overworld};
};

struct EnterBattleModeRequest {};

struct StartWildBattleRequest {
    int speciesId{0};
    int level{0};
};

struct StartTrainerBattleRequest {
    NPC *npc{nullptr};
};

struct EndBattleRequest {};

struct TransitionToMapRequest {
    std::string mapId;
    Position spawn{0, 0};
};

struct StartDialogueRequest {
    std::string speaker;
    std::vector<std::string> lines;
    NPC *npc{nullptr};
    GameState returnState{GameState::overworld};
};

struct StartDialogueChoiceRequest {
    std::vector<std::string> choiceOptions;
    std::string choiceContext;
    GameState returnState{GameState::overworld};
};

struct StartCutsceneRequest {
    std::string cutscenePath;
};

struct StoryActionRequest {
    StoryAction action;
};

struct ModeRequest {
    using Payload =
        std::variant<ChangeStateRequest, EnterBattleModeRequest, StartWildBattleRequest,
                     StartTrainerBattleRequest, EndBattleRequest, TransitionToMapRequest,
                     StartDialogueRequest, StartDialogueChoiceRequest, StartCutsceneRequest,
                     StoryActionRequest>;

    Payload payload;

    static ModeRequest changeState(GameState s) { return ModeRequest{ChangeStateRequest{s}}; }

    static ModeRequest enterBattleMode() { return ModeRequest{EnterBattleModeRequest{}}; }

    static ModeRequest wildBattle(int species, int level) {
        return ModeRequest{StartWildBattleRequest{species, level}};
    }

    static ModeRequest trainerBattle(NPC *trainer) {
        return ModeRequest{StartTrainerBattleRequest{trainer}};
    }

    static ModeRequest endBattle() { return ModeRequest{EndBattleRequest{}}; }

    static ModeRequest transition(std::string mapId, Position spawn) {
        return ModeRequest{TransitionToMapRequest{std::move(mapId), spawn}};
    }

    static ModeRequest dialogue(std::string speaker, std::vector<std::string> lines,
                                NPC *npc = nullptr,
                                GameState retState = GameState::overworld) {
        return ModeRequest{
            StartDialogueRequest{std::move(speaker), std::move(lines), npc, retState}};
    }

    static ModeRequest dialogueChoice(std::vector<std::string> options, std::string context,
                                      GameState retState = GameState::overworld) {
        return ModeRequest{StartDialogueChoiceRequest{
            std::move(options), std::move(context), retState}};
    }

    static ModeRequest cutscene(std::string path) {
        return ModeRequest{StartCutsceneRequest{std::move(path)}};
    }

    static ModeRequest storyAction(StoryAction action) {
        return ModeRequest{StoryActionRequest{std::move(action)}};
    }
};

// ── Shared context passed to every mode
// ────────────────────────────────────────

struct ModeMailbox {
    std::vector<ModeRequest> pending;

    void push(ModeRequest req) { pending.push_back(std::move(req)); }

    std::vector<ModeRequest> drain() {
        std::vector<ModeRequest> requests;
        std::swap(requests, pending);
        return requests;
    }
};

struct BattleSessionState {
    std::unique_ptr<Battle> active;
    std::string trainerNPCId;
};

struct SessionFlowState {
    int currentSaveSlot{-1};
    bool pendingPushBack{false};
    GameState dialogueReturnState{GameState::overworld};
};

struct GameContext {
    World &world;
    Pokedex &pokedex;
    GameUI &ui;
    SaveManager &saveManager;
    StoryManager &story;
    MusicManager &music;
    CutsceneRunner &cutsceneRunner;
    SoundManager &sound;

    // Shared mutable state (owned here, used by multiple modes)
    BattleSessionState battleSession;
    SessionFlowState flow;
    ModeMailbox mailbox;

    GameContext(World &world, Pokedex &pokedex, GameUI &ui, SaveManager &saveManager,
                StoryManager &story, MusicManager &music, CutsceneRunner &cutsceneRunner,
                SoundManager &sound);

    void setBattle(std::unique_ptr<Battle> battle, std::string trainerNPCId = {});
    bool hasBattle() const;
    Battle *tryBattle();
    const Battle *tryBattle() const;
    Battle &battle();
    const Battle &battle() const;
    const std::string &battleTrainerNPCId() const;
    void clearBattle();

    // Play a sound effect (convenience wrapper)
    void playSound(SoundEffect sfx);

    void pushRequest(ModeRequest req);

    ~GameContext();
};

// ── Abstract base for all game modes
// ───────────────────────────────────────────

class GameMode {
  public:
    virtual ~GameMode() = default;
    virtual void update(GameContext &ctx, InputManager &input) = 0;
    virtual void render(GameContext &ctx) = 0;
    virtual void onEnter(GameContext & /*ctx*/) {}
    virtual void onExit(GameContext & /*ctx*/) {}

  protected:
    // Convenience: render the overworld (many modes draw it as a backdrop)
    static void renderOverworld(GameContext &ctx);
};
