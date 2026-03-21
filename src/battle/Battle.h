/**
 * @file
 * @brief Battle state machine coordinating player choices, turn execution, and battle messages.
 * @ingroup battle_system
 */

#pragma once
#include "BattleEventQueue.h"
#include "BattleTypes.h"
#include "../game_data/Pokedex.h"
#include "../state/Daemon.h"
#include "../state/NPC.h"
#include "../state/player/Player.h"
#include <memory>
#include <random>
#include <vector>

/**
 * @brief Runtime battle coordinator for both wild and trainer battles.
 * @ingroup battle_system
 *
 * The battle owns the active wild daemon when needed, references the player and
 * optional trainer NPC, and advances through a message-driven state machine.
 */
class Battle {
  public:
    /// Creates a battle against a standalone opponent daemon.
    Battle(Player &player, std::unique_ptr<Daemon> opponent, BattleType type, std::mt19937 &rng,
           const Pokedex &pokedex);
    /// Creates a battle against a trainer NPC and the trainer's lead daemon.
    Battle(Player &player, NPC *opponent, BattleType type, std::mt19937 &rng,
           const Pokedex &pokedex);

    /// Queues the intro sequence and initial battle messages.
    void start();
    /// Chooses the top-level action for the current player turn.
    void chooseAction(BattleAction action);
    /// Selects a move slot and resolves the player turn.
    void chooseMove(int moveSlot);
    /// Uses an item from the bag within the current battle.
    void chooseItem(int itemId);
    /// Switches the active daemon to the chosen party index.
    void chooseSwitchTarget(int partyIndex);
    /// Returns from a battle submenu to the action-selection state.
    void goBack();

    /// Returns the currently active battle state.
    BattleState getState() const;
    /// Returns the state queued after the current message sequence completes.
    BattleState getPendingState() const;
    /// Returns the current win/loss/flee result projection.
    BattleResult getResult() const;

    /// Returns the player's currently active daemon.
    Daemon &getPlayerDaemon();
    /// Returns the opponent daemon currently on the field.
    Daemon &getOpponentDaemon();
    /// Returns the trainer NPC for trainer battles, or `nullptr` for wild battles.
    NPC *getOpponent() const;
    /// Returns the battle type.
    BattleType getType() const;

    /// Returns whether the player is allowed to flee.
    bool canFlee() const;
    /// Attempts a capture and queues the corresponding capture messages.
    bool attemptCapture(int itemId);

    /// Returns the current intro animation phase index.
    int getIntroPhase() const;
    /// Returns whether all intro phases have finished.
    bool isIntroComplete() const;

    /// Returns the current front message, or an empty string when idle.
    const std::string &getMessage() const;
    /// Returns whether messages or animation markers remain in the queue.
    bool hasMessages() const;
    /// Advances the message queue and transitions into animation states as needed.
    void advanceMessage();
    /// Resumes the battle flow after HP animation completes.
    void finishHPAnimation();
    /// Resumes the battle flow after EXP animation completes.
    void finishEXPAnimation();
    /// Resumes the battle flow after the intro animation completes.
    void finishIntroAnimation();
    /// Resumes the battle flow after the capture animation completes.
    void finishCaptureAnimation();
    /// Resumes the battle flow after the attack animation completes.
    void finishAttackAnimation();
    /// Resumes the battle flow after the player switch animation completes.
    void finishSwitchAnimation();
    /// Inserts a level-up message while keeping the EXP animation flow intact.
    void addLevelUpMessage(const std::string &msg);
    /// Executes the opponent's attack phase.
    void executeOpponentTurn();

    /// Returns the number of ball shakes for the active capture attempt.
    int getCaptureShakes() const;
    /// Returns whether the last capture attempt succeeded.
    bool getCaptureSuccess() const;
    /// Returns whether the current attack animation is from the player's daemon.
    bool isPlayerAttacking() const;
    /// Returns whether the active player switch animation is recalling the current daemon.
    bool isSwitchRecalling() const;
    /// Returns whether the given party index participated and therefore gained battle EXP.
    bool didPlayerParticipate(int partyIndex) const;

  private:
    void addMessage(const std::string &msg);
    void addHPAnimMarker();                  // Insert marker to trigger HP bar animation
    void addEXPAnimMarker();                 // Insert marker to trigger EXP bar animation
    void addIntroAnimMarker();               // Insert marker to continue intro animation
    void addCaptureAnimMarker();             // Insert marker to trigger capture animation
    void addAttackAnimMarker(bool isPlayer); // Insert marker to trigger attack animation
    void addSwitchAnimMarker(bool isRecall); // Insert marker to trigger player switch animation
    int participantCount() const;
    void transitionToQueuedState();
    void executeTurn();

    Player &player;
    std::unique_ptr<Daemon> opponentDaemon = nullptr;
    NPC *opponent = nullptr;       ///< Non-owning trainer pointer for trainer battles.
    BattleType type;               ///< Wild or trainer battle classification.
    BattleState state;             ///< Current state of the battle state machine.
    BattleState pendingState;      ///< State entered after the current message queue drains.
    int introPhase{0};             ///< Current intro animation phase.
    bool introComplete{false};     ///< Whether intro animation has fully completed.
    int captureShakes{0};          ///< Number of ball shakes queued for capture animation.
    bool captureSuccess{false};    ///< Whether the queued capture attempt succeeds.
    bool attackAnimIsPlayer{true}; ///< Whether the active attack animation is from the player.
    bool switchAnimIsRecall{false}; ///< Whether the active switch animation is a recall phase.

    int playerMoveSlot;
    int itemChoice;
    int switchTarget;
    BattleAction currentAction;
    int pendingPlayerSwitchIndex{-1};
    int expGained{0};
    int moneyGained{0};

    std::mt19937 &rng;
    const Pokedex &pokedex;
    BattleEventQueue eventQueue;
    std::vector<bool> playerParticipants;
    std::string emptyMessage;      ///< Empty string returned when no message is available.
};
