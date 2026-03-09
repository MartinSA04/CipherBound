#pragma once
#include <random>
#include <deque>
#include "../world/Daemon.h"
#include "../world/Player.h"
#include "../world/NPC.h"
#include "../data/Pokedex.h"
#include <memory>

enum class BattleState
{
    intro,
    choosingAction,
    choosingMove,
    choosingItem,
    choosingSwitch,
    showingMessages,
    animatingHP,
    animatingEXP,
    opponentTurn,
    victory,
    defeat,
    fled,
    captured,
};

enum class BattleAction
{
    fight,
    item,
    switchDaemon,
    flee,
};

struct BattleResult
{
    bool playerWon;
    bool playerFled;
    bool captured;
    int expGained;
    int moneyGained;
};

enum class BattleType
{
    wild,
    trainer,
    gymLeader,
};

class Battle
{
public:
    Battle(Player &player, std::unique_ptr<Daemon> opponent, BattleType type, std::mt19937 &rng, const Pokedex &pokedex);
    Battle(Player &player, std::shared_ptr<NPC> opponent, BattleType type, std::mt19937 &rng, const Pokedex &pokedex);

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
    std::shared_ptr<NPC> getOpponent();
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
    void addLevelUpMessage(const std::string &msg); // Insert level-up msg + resume EXP anim
    void executeOpponentTurn();                     // Execute the opponent's attack phase

private:
    void addMessage(const std::string &msg);
    void addHPAnimMarker();    // Insert marker to trigger HP bar animation
    void addEXPAnimMarker();   // Insert marker to trigger EXP bar animation
    void addIntroAnimMarker(); // Insert marker to continue intro animation
    void executeTurn();

    int calculateDamage(const Daemon &attacker, const Daemon &defender, const MoveData &move) const;
    float getTypeEffectiveness(ElementType attackType, ElementType defenseType) const;
    bool accuracyCheck(int accuracy) const;
    int calculateExpYield(const Daemon &defeated) const;

    Player &player;
    std::unique_ptr<Daemon> opponentDaemon = nullptr;
    std::shared_ptr<NPC> opponent = nullptr;
    BattleType type;
    BattleState state;
    BattleState pendingState;  // State to go to after message queue is drained
    int introPhase{0};         // Which intro animation phase we're in
    bool introComplete{false}; // True once all intro phases are done

    int playerMoveSlot;
    int itemChoice;
    int switchTarget;
    BattleAction currentAction;

    std::mt19937 &rng;
    const Pokedex &pokedex;
    std::deque<std::string> messages;
    std::string emptyMessage; // Returned when queue is empty
    static constexpr const char *HP_ANIM_MARKER = "__HP_ANIM__";
    static constexpr const char *EXP_ANIM_MARKER = "__EXP_ANIM__";
    static constexpr const char *INTRO_ANIM_MARKER = "__INTRO_ANIM__";
};
