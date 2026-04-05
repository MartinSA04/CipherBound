/**
 * @file
 * @brief Shared app-flow types for modes, requests, and runtime session state.
 * @ingroup app_core
 */

#pragma once
#include "../audio/SoundEffects.h"
#include "../battle/ui/BattlePresentationState.h"
#include "../game_data/Pokedex.h"
#include "../state/World.h"
#include "../story/StoryAction.h"
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

class NPC;
class MusicManager;
enum class MusicTrack;
class CutsceneRunner;
class Battle;
class SaveManager;
class GameUI;
class InputManager;
class SoundManager;
class StoryManager;

/**
 * @brief High-level states managed by the session coordinator.
 * @ingroup app_core
 */
enum class GameState {
    titleScreen,    ///< Title screen and save-slot selection.
    overworld,      ///< Free movement in the world map.
    battle,         ///< Main turn-based battle loop.
    battleIntro,    ///< Battle entry animation and setup.
    menu,           ///< Pause menu.
    playerStats,    ///< Trainer stats and badge overview.
    party,          ///< Party management screen.
    bag,            ///< Inventory screen.
    shop,           ///< Shop interaction screen.
    saving,         ///< Save flow UI.
    dialogue,       ///< Linear dialogue box.
    dialogueChoice, ///< Dialogue choice prompt.
    transition,     ///< Fade or blackout transition.
    pcBox,          ///< PC storage management.
    cutscene,       ///< Scripted cutscene playback.
    daemondex,      ///< Daemondex viewer.
    daemonNaming,   ///< Naming prompt for a newly obtained daemon.
};

/// Requests a direct state switch to an existing mode.
struct ChangeStateRequest {
    GameState targetState{GameState::overworld}; ///< Target state to activate.
};

/// Requests entry into the active battle mode after setup has completed.
struct EnterBattleModeRequest {};

/// Requests a wild battle against a generated daemon.
struct StartWildBattleRequest {
    int speciesId{0}; ///< Species id loaded from the pokedex data set.
    int level{0};     ///< Level used when creating the opponent daemon.
};

/// Requests a trainer battle and optional pre-battle cutscene.
struct StartTrainerBattleRequest {
    NPC *npc{nullptr};                    ///< Non-owning pointer to the trainer NPC.
    bool includePreBattleDialogue{false}; ///< Whether the eyes-meet cutscene should run first.
};

/// Requests the battle-intro phase for a specific trainer.
struct StartTrainerBattleIntroRequest {
    NPC *npc{nullptr}; ///< Non-owning pointer to the trainer NPC.
};

/// Requests teardown of the current battle session.
struct EndBattleRequest {};

/// Requests a map transition and spawn position update.
struct TransitionToMapRequest {
    std::string mapId;    ///< Destination map id.
    Position spawn{0, 0}; ///< Spawn tile on the destination map.
};

/// Requests a dialogue screen with explicit return-state information.
struct StartDialogueRequest {
    std::string speaker;                         ///< Speaker name shown in the UI.
    std::vector<std::string> lines;              ///< Dialogue lines displayed in order.
    NPC *npc{nullptr};                           ///< Optional NPC associated with the dialogue.
    GameState returnState{GameState::overworld}; ///< State to resume after dialogue closes.
};

/// Requests a dialogue choice prompt.
struct StartDialogueChoiceRequest {
    std::vector<std::string> choiceOptions;      ///< Ordered list of choices shown to the player.
    std::string choiceContext;                   ///< Story context key used to resolve the choice.
    GameState returnState{GameState::overworld}; ///< State to resume after the choice.
};

/// Follow-up flow for a newly obtained daemon naming prompt.
enum class DaemonNamingPurpose {
    starter,      ///< Naming a newly selected starter daemon.
    battleCapture ///< Naming a daemon caught in battle.
};

/// Requests a naming prompt for a newly obtained daemon.
struct StartDaemonNamingRequest {
    Daemon daemon;                               ///< Daemon instance being named.
    DaemonNamingPurpose purpose;                 ///< Why the naming prompt was opened.
    std::string completionSpeaker;               ///< Optional speaker for post-name dialogue.
    std::vector<std::string> completionLines;    ///< Optional lines shown after naming.
    GameState returnState{GameState::overworld}; ///< State to resume after any follow-up dialogue.
};

/// Requests that the shop UI opens with a concrete inventory.
struct OpenShopRequest {
    std::string title;          ///< UI title for the shop screen.
    std::string shopkeeperName; ///< Display name of the shopkeeper.
    std::vector<int> itemIds;   ///< Item ids offered for sale.
};

/// Requests loading and playback of a cutscene file.
struct StartCutsceneRequest {
    std::string cutscenePath; ///< Relative or resolved path to a `.cutscene` file.
};

/// Wraps a story-layer action so it can be processed by the coordinator.
struct StoryActionRequest {
    StoryAction action; ///< Concrete story action payload.
};

/**
 * @brief Type-erased request envelope exchanged between modes and the session coordinator.
 * @ingroup app_core
 */
struct ModeRequest {
    /// Variant containing every supported cross-mode request type.
    using Payload =
        std::variant<ChangeStateRequest, EnterBattleModeRequest, StartWildBattleRequest,
                     StartTrainerBattleRequest, StartTrainerBattleIntroRequest, EndBattleRequest,
                     TransitionToMapRequest, StartDialogueRequest, StartDialogueChoiceRequest,
                     StartDaemonNamingRequest, OpenShopRequest, StartCutsceneRequest,
                     StoryActionRequest>;

    Payload payload; ///< Concrete request stored in this envelope.

    /// Builds a state-change request.
    static ModeRequest changeState(GameState s) { return ModeRequest{ChangeStateRequest{s}}; }

    /// Builds an enter-battle request.
    static ModeRequest enterBattleMode() { return ModeRequest{EnterBattleModeRequest{}}; }

    /// Builds a wild-battle request.
    static ModeRequest wildBattle(int species, int level) {
        return ModeRequest{StartWildBattleRequest{species, level}};
    }

    /// Builds a trainer-battle request.
    static ModeRequest trainerBattle(NPC *trainer, bool includePreBattleDialogue = false) {
        return ModeRequest{StartTrainerBattleRequest{trainer, includePreBattleDialogue}};
    }

    /// Builds a trainer battle-intro request.
    static ModeRequest trainerBattleIntro(NPC *trainer) {
        return ModeRequest{StartTrainerBattleIntroRequest{trainer}};
    }

    /// Builds a battle-end request.
    static ModeRequest endBattle() { return ModeRequest{EndBattleRequest{}}; }

    /// Builds a map-transition request.
    static ModeRequest transition(std::string mapId, Position spawn) {
        return ModeRequest{TransitionToMapRequest{std::move(mapId), spawn}};
    }

    /// Builds a dialogue request.
    static ModeRequest dialogue(std::string speaker, std::vector<std::string> lines,
                                NPC *npc = nullptr, GameState retState = GameState::overworld) {
        return ModeRequest{
            StartDialogueRequest{std::move(speaker), std::move(lines), npc, retState}};
    }

    /// Builds a dialogue-choice request.
    static ModeRequest dialogueChoice(std::vector<std::string> options, std::string context,
                                      GameState retState = GameState::overworld) {
        return ModeRequest{
            StartDialogueChoiceRequest{std::move(options), std::move(context), retState}};
    }

    /// Builds a daemon-naming request.
    static ModeRequest daemonNaming(Daemon daemon, DaemonNamingPurpose purpose,
                                    std::string speaker = {},
                                    std::vector<std::string> lines = {},
                                    GameState retState = GameState::overworld) {
        return ModeRequest{StartDaemonNamingRequest{std::move(daemon), purpose, std::move(speaker),
                                                    std::move(lines), retState}};
    }

    /// Builds a shop-open request.
    static ModeRequest openShop(std::string title, std::string shopkeeperName,
                                std::vector<int> itemIds) {
        return ModeRequest{
            OpenShopRequest{std::move(title), std::move(shopkeeperName), std::move(itemIds)}};
    }

    /// Builds a cutscene-start request.
    static ModeRequest cutscene(std::string path) {
        return ModeRequest{StartCutsceneRequest{std::move(path)}};
    }

    /// Builds a story-action request.
    static ModeRequest storyAction(StoryAction action) {
        return ModeRequest{StoryActionRequest{std::move(action)}};
    }
};

/**
 * @brief Queue of requests emitted by modes during update or render.
 * @ingroup app_core
 */
struct ModeMailbox {
    std::vector<ModeRequest> pending; ///< Requests waiting to be processed.

    /// Appends a request to the mailbox.
    void push(ModeRequest req) { pending.push_back(std::move(req)); }

    /// Drains all pending requests and leaves the mailbox empty.
    std::vector<ModeRequest> drain() {
        std::vector<ModeRequest> requests;
        std::swap(requests, pending);
        return requests;
    }
};

/// Runtime battle-related state shared across multiple modes.
struct BattleSessionState {
    std::unique_ptr<Battle> active;       ///< Owned active battle, if any.
    std::string trainerNPCId;             ///< Trainer id tied to the current battle, if any.
    BattlePresentationState presentation; ///< UI/presentation state reused across battle phases.
};

/// Session flags that survive mode switches outside the world model itself.
struct SessionFlowState {
    int currentSaveSlot{-1};     ///< Save slot currently selected by the player.
    bool pendingPushBack{false}; ///< Whether dialogue exit should push the player backward.
    GameState dialogueReturnState{GameState::overworld}; ///< State to resume after dialogue.
    std::optional<ModeRequest>
        cutsceneEndRequest; ///< Optional request dispatched when a cutscene ends.
};

/**
 * @brief Shared mutable runtime context passed into every mode callback.
 * @ingroup app_core
 *
 * The subsystem references are non-owning and are expected to outlive the
 * context for the duration of a session.
 */
struct GameContext {
    World &world;                   ///< World state and map ownership.
    Pokedex &pokedex;               ///< Loaded game-data catalog.
    GameUI &ui;                     ///< High-level UI facade and renderer access.
    SaveManager &saveManager;       ///< Save/load orchestration.
    StoryManager &story;            ///< Story progression logic.
    MusicManager &music;            ///< Background music playback.
    CutsceneRunner &cutsceneRunner; ///< Scripted cutscene executor.
    SoundManager &sound;            ///< Sound effect playback.

    BattleSessionState battleSession; ///< Shared battle session state.
    SessionFlowState flow;            ///< Cross-mode flow flags.
    ModeMailbox mailbox;              ///< Pending mode requests.

    /// Constructs a context from long-lived subsystem references.
    GameContext(World &world, Pokedex &pokedex, GameUI &ui, SaveManager &saveManager,
                StoryManager &story, MusicManager &music, CutsceneRunner &cutsceneRunner,
                SoundManager &sound);

    /// Replaces the active battle and optional trainer binding.
    void setBattle(std::unique_ptr<Battle> battle, std::string trainerNPCId = {});
    /// Returns whether a battle is currently active.
    bool hasBattle() const;
    /// Returns the active battle or `nullptr` if none exists.
    Battle *tryBattle();
    /// Returns the active battle or `nullptr` if none exists.
    const Battle *tryBattle() const;
    /// Returns the active battle or throws if no battle is active.
    Battle &battle();
    /// Returns the active battle or throws if no battle is active.
    const Battle &battle() const;
    /// Returns the trainer id associated with the active battle, if any.
    const std::string &battleTrainerNPCId() const;
    /// Returns mutable presentation state for the active battle flow.
    BattlePresentationState &battlePresentation();
    /// Returns immutable presentation state for the active battle flow.
    const BattlePresentationState &battlePresentation() const;
    /// Resets only the battle presentation layer.
    void resetBattlePresentation();
    /// Clears the active battle and associated presentation state.
    void clearBattle();

    /// Plays a sound effect through the shared sound manager.
    void playSound(SoundEffect sfx);
    /// Plays looping background music through the shared music manager.
    void playMusic(MusicTrack track);
    /// Plays a one-shot music cue through the shared music manager.
    void playMusicOneShot(MusicTrack track);
    /// Plays the current map's configured looping music path, if any.
    void playCurrentMapMusic();
    /// Stops the currently playing music track.
    void stopMusic();

    /// Pushes a coordinator request into the shared mailbox.
    void pushRequest(ModeRequest req);

    /// Destroys the context without taking ownership of subsystem references.
    ~GameContext();
};

/**
 * @brief Abstract base class implemented by each concrete game mode.
 * @ingroup app_core
 */
class GameMode {
  public:
    /// Virtual destructor for polymorphic mode ownership.
    virtual ~GameMode() = default;
    /// Advances mode logic by one frame.
    virtual void update(GameContext &ctx, InputManager &input) = 0;
    /// Draws the current mode.
    virtual void render(GameContext &ctx) = 0;
    /// Hook called immediately after the mode becomes active.
    virtual void onEnter(GameContext & /*ctx*/) {}
    /// Hook called immediately before the mode is replaced.
    virtual void onExit(GameContext & /*ctx*/) {}

  protected:
    /// Draws the overworld scene as a backdrop for overlay-style modes.
    static void renderOverworld(GameContext &ctx);
};
