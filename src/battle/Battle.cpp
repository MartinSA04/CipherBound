#include "Battle.h"
#include "BattleCapture.h"
#include "BattleRules.h"
#include "BattleTurnResolver.h"
#include <string_view>

namespace {

constexpr int fullRestoreThreshold = 9999;

std::string_view effectivenessMessage(float effectiveness) {
    if (effectiveness >= 2.0f)
        return "It's super effective!";
    if (effectiveness > 0.0f && effectiveness <= 0.5f)
        return "It's not very effective...";
    if (effectiveness == 0.0f)
        return "It has no effect!";
    return {};
}

} // namespace

Battle::Battle(Player &player, std::unique_ptr<Daemon> opponent, BattleType type, std::mt19937 &rng,
               const Pokedex &pokedex)
    : player(player), opponentDaemon(std::move(opponent)), type(type), state(BattleState::intro),
      pendingState(BattleState::choosingAction), playerMoveSlot(0), itemChoice(0), switchTarget(0),
      currentAction(BattleAction::fight), rng(rng), pokedex(pokedex) {}
Battle::Battle(Player &player, NPC *opponent, BattleType type, std::mt19937 &rng,
               const Pokedex &pokedex)
    : player(player), opponent(opponent), type(type), state(BattleState::intro),
      pendingState(BattleState::choosingAction), playerMoveSlot(0), itemChoice(0), switchTarget(0),
      currentAction(BattleAction::fight), rng(rng), pokedex(pokedex) {}

void Battle::start() {
    // Battle already starts in BattleState::intro (from constructor).
    // Phase 0: opponent (Daemon or NPC) slides in.
    introPhase = 0;

    if (opponent == nullptr) {
        // Wild: after phase 0 anim, show message then player sends out
        addMessage("A wild " + getOpponentDaemon().getNickname() + " appeared!");
    } else {
        // Trainer: after phase 0 (NPC appears), show lines, then phase 1 (NPC
        // out, Daemon in)
        addMessage(opponent->getName() + " wants to fight!");
        addMessage(opponent->getName() + " sent out " + getOpponentDaemon().getNickname() + "!");
        addIntroAnimMarker(); // phase 1: NPC slides out, their Daemon comes in
    }

    addMessage("Go " + getPlayerDaemon().getNickname() + "!");
    addIntroAnimMarker(); // next phase: player Daemon comes in

    pendingState = BattleState::choosingAction;
    // state stays as BattleState::intro (set by constructor)
}

void Battle::chooseAction(BattleAction action) {
    currentAction = action;
    switch (action) {
    case BattleAction::fight:
        state = BattleState::choosingMove;
        break;
    case BattleAction::item:
        state = BattleState::choosingItem;
        break;
    case BattleAction::switchDaemon:
        state = BattleState::choosingSwitch;
        break;
    case BattleAction::flee:
        if (canFlee()) {
            addMessage("Got away safely!");
            pendingState = BattleState::fled;
            state = BattleState::showingMessages;
        } else {
            addMessage("Can't escape!");
            pendingState = BattleState::choosingAction;
            state = BattleState::showingMessages;
        }
        break;
    }
}

void Battle::chooseMove(int moveSlot) {
    playerMoveSlot = moveSlot;
    executeTurn();
}

void Battle::chooseItem(int itemId) {
    itemChoice = itemId;
    const ItemData &item = pokedex.getItem(itemId);

    if (item.category == ItemCategory::healing) {
        Daemon &pc = getPlayerDaemon();
        if (pc.getCurrentHP() >= pc.getMaxHP()) {
            addMessage("HP is already full!");
            pendingState = BattleState::choosingItem;
            state = BattleState::showingMessages;
            return;
        }

        // Remove item from inventory
        player.removeItem(itemId, 1);

        int healBefore = pc.getCurrentHP();
        if (item.effectValue >= fullRestoreThreshold)
            pc.fullHeal();
        else
            pc.heal(item.effectValue);
        int healed = pc.getCurrentHP() - healBefore;

        addMessage("Used " + item.name + "!");
        addHPAnimMarker();
        addMessage(pc.getNickname() + " recovered " + std::to_string(healed) + " HP!");

        // Opponent gets a turn after using an item
        pendingState = BattleState::opponentTurn;
        state = BattleState::showingMessages;
    } else if (item.category == ItemCategory::capture) {
        // Remove item from inventory
        player.removeItem(itemId, 1);
        attemptCapture(itemId);
    } else {
        addMessage("Can't use that here!");
        pendingState = BattleState::choosingItem;
        state = BattleState::showingMessages;
    }
}

void Battle::chooseSwitchTarget(int partyIndex) {
    switchTarget = partyIndex;

    if (partyIndex == 0) {
        addMessage("That Daemon is already out!");
        pendingState = BattleState::choosingSwitch;
        state = BattleState::showingMessages;
        return;
    }

    Daemon &target = player.getDaemon(partyIndex);
    if (target.isFainted()) {
        addMessage(target.getNickname() + " has no energy left!");
        pendingState = BattleState::choosingSwitch;
        state = BattleState::showingMessages;
        return;
    }

    // Swap the Daemon to the front
    std::string oldName = player.getDaemon(0).getNickname();
    player.swapDaemon(0, partyIndex);
    addMessage("Come back, " + oldName + "!");
    addMessage("Go, " + player.getDaemon(0).getNickname() + "!");

    // Opponent gets a turn after switching
    pendingState = BattleState::opponentTurn;
    state = BattleState::showingMessages;
}

void Battle::goBack() {
    if (state == BattleState::choosingMove || state == BattleState::choosingItem ||
        state == BattleState::choosingSwitch) {
        state = BattleState::choosingAction;
    }
}

void Battle::executeTurn() {
    Daemon &playerDaemon = player.getDaemon(0); // active Daemon

    if (currentAction == BattleAction::fight) {
        const BattleMoveSelection selection =
            BattleTurnResolver::preparePlayerMove(playerDaemon, playerMoveSlot, pokedex);

        if (selection.error == BattleMoveSelectionError::invalidSelection) {
            addMessage("No move selected!");
            pendingState = BattleState::choosingAction;
            state = BattleState::showingMessages;
            return;
        }
        if (selection.error == BattleMoveSelectionError::noPP) {
            addMessage("No PP left for that move!");
            pendingState = BattleState::choosingAction;
            state = BattleState::showingMessages;
            return;
        }

        const MoveData &moveData = *selection.move;
        const BattleAttackResolution resolution =
            BattleTurnResolver::resolveAttack(playerDaemon, getOpponentDaemon(), moveData, rng);

        if (!resolution.hit) {
            addMessage(playerDaemon.getNickname() + " used " + moveData.name + "!");
            addMessage("But it missed!");
            pendingState = BattleState::opponentTurn;
            state = BattleState::showingMessages;
            return;
        }

        addMessage(playerDaemon.getNickname() + " used " + moveData.name + "!");
        addAttackAnimMarker(true);
        addHPAnimMarker();

        if (const std::string_view message = effectivenessMessage(resolution.effectiveness);
            !message.empty()) {
            addMessage(std::string(message));
        }

        addMessage("It dealt " + std::to_string(resolution.damage) + " damage!");

        if (resolution.defenderFainted) {
            expGained = BattleRules::calculateExpYield(getOpponentDaemon());
            moneyGained = BattleRules::calculateMoneyReward(getOpponentDaemon(), type);
            playerDaemon.addExp(expGained);
            if (moneyGained > 0)
                player.addMoney(moneyGained);
            addMessage("The opposing " + getOpponentDaemon().getNickname() + " fainted!");
            addMessage("Gained " + std::to_string(expGained) + " EXP!");
            addEXPAnimMarker();
            if (moneyGained > 0)
                addMessage("Received " + std::to_string(moneyGained) + " dollars!");
            addMessage("You won!");
            pendingState = BattleState::victory;
            state = BattleState::showingMessages;
            return;
        }

        // After player's messages, transition to opponent's turn
        pendingState = BattleState::opponentTurn;
        state = BattleState::showingMessages;
        return;
    }

    pendingState = BattleState::choosingAction;
    state = BattleState::showingMessages;
}

void Battle::executeOpponentTurn() {
    Daemon &playerDaemon = player.getDaemon(0);
    BattleMoveSelection selection =
        BattleTurnResolver::prepareOpponentMove(getOpponentDaemon(), playerDaemon, pokedex, rng);
    if (selection.error != BattleMoveSelectionError::none || selection.move == nullptr) {
        pendingState = BattleState::choosingAction;
        state = BattleState::showingMessages;
        return;
    }

    const MoveData &oppMoveData = *selection.move;
    const BattleAttackResolution resolution =
        BattleTurnResolver::resolveAttack(getOpponentDaemon(), playerDaemon, oppMoveData, rng);

    if (!resolution.hit) {
        addMessage("Foe " + getOpponentDaemon().getNickname() + " used " + oppMoveData.name + "!");
        addMessage("But it missed!");
        pendingState = BattleState::choosingAction;
        state = BattleState::showingMessages;
        return;
    }

    addMessage("Foe " + getOpponentDaemon().getNickname() + " used " + oppMoveData.name + "!");
    addAttackAnimMarker(false);
    addHPAnimMarker();

    if (const std::string_view message = effectivenessMessage(resolution.effectiveness);
        !message.empty()) {
        addMessage(std::string(message));
    }

    addMessage("It dealt " + std::to_string(resolution.damage) + " damage!");

    if (resolution.defenderFainted) {
        addMessage(playerDaemon.getNickname() + " fainted!");
        const int replacementIndex = player.findFirstUsableDaemonIndex(1);
        if (replacementIndex >= 0) {
            pendingAutoSwitchIndex = replacementIndex;
            pendingState = BattleState::choosingAction;
        } else {
            pendingAutoSwitchIndex = -1;
            addMessage("You blacked out!");
            pendingState = BattleState::defeat;
        }
    } else {
        pendingAutoSwitchIndex = -1;
        pendingState = BattleState::choosingAction;
    }
    state = BattleState::showingMessages;
}

BattleState Battle::getState() const { return state; }
BattleState Battle::getPendingState() const { return pendingState; }

BattleResult Battle::getResult() const {
    BattleResult result{};
    result.playerWon = (state == BattleState::victory || state == BattleState::captured);
    result.playerFled = (state == BattleState::fled);
    result.captured = (state == BattleState::captured);
    result.expGained = expGained;
    result.moneyGained = moneyGained;
    return result;
}

Daemon &Battle::getPlayerDaemon() { return player.getDaemon(0); }

Daemon &Battle::getOpponentDaemon() {
    if (opponent == nullptr)
        return *opponentDaemon;
    else
        return opponent->getDaemon(0);
}

NPC *Battle::getOpponent() const { return opponent; }

BattleType Battle::getType() const { return type; }

bool Battle::canFlee() const { return type == BattleType::wild; }

bool Battle::attemptCapture(int itemId) {
    if (opponent != nullptr) {
        addMessage("Cannot capture opponent's Daemons!");
        pendingState = BattleState::choosingAction;
        state = BattleState::showingMessages;
        return false;
    }

    const Daemon &target = getOpponentDaemon();
    const ItemData &ball = pokedex.getItem(itemId);
    const BattleCaptureOutcome outcome = BattleCapture::resolve(target, ball, rng);

    captureShakes = outcome.shakes;
    captureSuccess = outcome.success;
    addMessage("Used " + ball.name + "!");
    addCaptureAnimMarker();

    if (outcome.success) {
        addMessage("Gotcha! " + target.getNickname() + " was caught!");
        pendingState = BattleState::captured;
        state = BattleState::showingMessages;

        player.addDaemon(BattleCapture::caughtDaemon(target));
        player.markCaught(target.getSpeciesId());
        return true;
    }

    addMessage(std::string(BattleCapture::failureMessage(outcome.shakes)));

    // Opponent gets a turn after failed capture
    pendingState = BattleState::opponentTurn;
    state = BattleState::showingMessages;
    return false;
}

const std::string &Battle::getMessage() const {
    if (const std::string *message = eventQueue.currentMessage())
        return *message;
    return emptyMessage;
}

bool Battle::hasMessages() const { return !eventQueue.empty(); }

void Battle::advanceMessage() {
    eventQueue.popCurrentMessage();
    transitionToQueuedState();
}

void Battle::finishIntroAnimation() {
    transitionToQueuedState();
    if (eventQueue.empty()) {
        introComplete = true;
    }
}

int Battle::getIntroPhase() const { return introPhase; }

bool Battle::isIntroComplete() const { return introComplete; }

void Battle::finishHPAnimation() { transitionToQueuedState(); }

void Battle::finishEXPAnimation() { transitionToQueuedState(); }

void Battle::finishCaptureAnimation() { transitionToQueuedState(); }

int Battle::getCaptureShakes() const { return captureShakes; }

bool Battle::getCaptureSuccess() const { return captureSuccess; }

void Battle::addLevelUpMessage(const std::string &msg) {
    // Show the message, then resume EXP animation for remaining EXP.
    eventQueue.pushLevelUpResume(msg);
    state = BattleState::showingMessages;
}

void Battle::addMessage(const std::string &msg) { eventQueue.pushMessage(msg); }

void Battle::addHPAnimMarker() { eventQueue.pushHPAnimation(); }

void Battle::addEXPAnimMarker() { eventQueue.pushEXPAnimation(); }

void Battle::addIntroAnimMarker() { eventQueue.pushIntroAnimation(); }

void Battle::addCaptureAnimMarker() { eventQueue.pushCaptureAnimation(); }

void Battle::addAttackAnimMarker(bool isPlayer) { eventQueue.pushAttackAnimation(isPlayer); }

void Battle::transitionToQueuedState() {
    state = eventQueue.consume(pendingState, introPhase, attackAnimIsPlayer);
    if (state == pendingState && eventQueue.empty() && pendingAutoSwitchIndex >= 0) {
        player.swapDaemon(0, pendingAutoSwitchIndex);
        pendingAutoSwitchIndex = -1;
        addMessage("Go, " + player.getDaemon(0).getNickname() + "!");
        state = BattleState::showingMessages;
    }
}

void Battle::finishAttackAnimation() { transitionToQueuedState(); }

bool Battle::isPlayerAttacking() const { return attackAnimIsPlayer; }
