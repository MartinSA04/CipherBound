#pragma once
#include "../audio/SoundManager.h"
#include "../data/Pokedex.h"
#include "../state/World.h"
#include "StoryManager.h"
#include <memory>
#include <string>
#include <vector>

// Forward declarations for types not included through StoryManager.h
class NPC;
class MusicManager;
class CutsceneRunner;
class Battle;
class SaveManager;
class GameUI;
class InputManager;

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

struct ModeRequest {
    enum class Type {
        changeState,
        startWildBattle,
        startTrainerBattle,
        endBattle,
        transitionToMap,
        startDialogue,
        startDialogueChoice,
        startCutscene,
        handleStoryAction,
    };

    Type type;
    GameState targetState{GameState::overworld};

    // Battle params
    int speciesId{0};
    int level{0};
    std::shared_ptr<NPC> npc;

    // Map params
    std::string mapId;
    Position spawn{0, 0};

    // Dialogue params
    std::string speaker;
    std::vector<std::string> lines;
    GameState returnState{GameState::overworld};

    // Dialogue choice params
    std::vector<std::string> choiceOptions;
    std::string choiceContext;

    // Cutscene params
    std::string cutscenePath;

    // Story action
    StoryAction storyActionData;

    // --- Factory helpers ---
    static ModeRequest changeState(GameState s);
    static ModeRequest wildBattle(int species, int level);
    static ModeRequest trainerBattle(std::shared_ptr<NPC> t);
    static ModeRequest endBattle();
    static ModeRequest transition(const std::string &mapId, const Position &spawn);
    static ModeRequest dialogue(const std::string &speaker, const std::vector<std::string> &lines,
                                std::shared_ptr<NPC> npc = nullptr,
                                GameState retState = GameState::overworld);
    static ModeRequest dialogueChoice(const std::vector<std::string> &options,
                                      const std::string &context,
                                      GameState retState = GameState::overworld);
    static ModeRequest cutscene(const std::string &path);
    static ModeRequest storyAction(const StoryAction &action);
};

// ── Shared context passed to every mode
// ────────────────────────────────────────

struct ModeMailbox {
    std::vector<ModeRequest> pending;

    void push(ModeRequest req);
    std::vector<ModeRequest> drain();
};

struct BattleSessionState {
    std::unique_ptr<Battle> active;
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

    void setBattle(std::unique_ptr<Battle> battle);
    bool hasBattle() const;
    Battle *tryBattle();
    const Battle *tryBattle() const;
    Battle &battle();
    const Battle &battle() const;
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
