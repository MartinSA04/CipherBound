#pragma once
#include "BattleEventQueue.h"
#include "BattleTypes.h"
#include "../game_data/Pokedex.h"
#include "../state/Daemon.h"
#include "../state/NPC.h"
#include "../state/player/Player.h"
#include <memory>
#include <random>

class Battle {
  public:
    Battle(Player &player, std::unique_ptr<Daemon> opponent, BattleType type, std::mt19937 &rng,
           const Pokedex &pokedex);
    Battle(Player &player, NPC *opponent, BattleType type, std::mt19937 &rng,
           const Pokedex &pokedex);

    void start();
    void chooseAction(BattleAction action);
    void chooseMove(int moveSlot);
    void chooseItem(int itemId);
    void chooseSwitchTarget(int partyIndex);
    void goBack(); // Return to choosingAction from sub-menus

    BattleState getState() const;
    BattleState getPendingState() const;
    BattleResult getResult() const;

    Daemon &getPlayerDaemon();
    Daemon &getOpponentDaemon();
    NPC *getOpponent() const;
    BattleType getType() const;

    bool canFlee() const;
    bool attemptCapture(int itemId);

    // Intro animation phase (incremented each time an INTRO_ANIM_MARKER is hit)
    int getIntroPhase() const;
    bool isIntroComplete() const;

    // Message queue
    const std::string &getMessage() const;
    bool hasMessages() const;
    void advanceMessage();                          // Pop front; if marker hit, go to animatingHP
    void finishHPAnimation();                       // Called when HP animation completes
    void finishEXPAnimation();                      // Called when EXP animation completes
    void finishIntroAnimation();                    // Called when intro scene animation completes
    void finishCaptureAnimation();                  // Called when capture animation completes
    void finishAttackAnimation();                   // Called when attack animation completes
    void addLevelUpMessage(const std::string &msg); // Insert level-up msg + resume EXP anim
    void executeOpponentTurn();                     // Execute the opponent's attack phase

    // Capture animation data
    int getCaptureShakes() const;   // How many shakes before break/catch
    bool getCaptureSuccess() const; // Did the capture succeed?
    bool isPlayerAttacking() const; // Is the player the attacker in current attack anim?

  private:
    void addMessage(const std::string &msg);
    void addHPAnimMarker();                  // Insert marker to trigger HP bar animation
    void addEXPAnimMarker();                 // Insert marker to trigger EXP bar animation
    void addIntroAnimMarker();               // Insert marker to continue intro animation
    void addCaptureAnimMarker();             // Insert marker to trigger capture animation
    void addAttackAnimMarker(bool isPlayer); // Insert marker to trigger attack animation
    void transitionToQueuedState();
    void executeTurn();

    Player &player;
    std::unique_ptr<Daemon> opponentDaemon = nullptr;
    NPC *opponent = nullptr; // non-owning; world keeps NPC storage alive
    BattleType type;
    BattleState state;
    BattleState pendingState;      // State to go to after message queue is drained
    int introPhase{0};             // Which intro animation phase we're in
    bool introComplete{false};     // True once all intro phases are done
    int captureShakes{0};          // How many shakes the ball does
    bool captureSuccess{false};    // Did the capture succeed?
    bool attackAnimIsPlayer{true}; // Is the attacker the player?

    int playerMoveSlot;
    int itemChoice;
    int switchTarget;
    BattleAction currentAction;

    std::mt19937 &rng;
    const Pokedex &pokedex;
    BattleEventQueue eventQueue;
    std::string emptyMessage; // Returned when queue is empty
};
