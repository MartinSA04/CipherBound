#include "Battle.h"
#include "TypeChart.h"
#include <algorithm>
#include <cmath>

Battle::Battle(Player &player, std::unique_ptr<Daemon> opponent, BattleType type, std::mt19937 &rng, const Pokedex &pokedex)
    : player(player), opponentDaemon(std::move(opponent)), type(type), state(BattleState::intro),
      pendingState(BattleState::choosingAction),
      playerMoveSlot(0), itemChoice(0), switchTarget(0),
      currentAction(BattleAction::fight), rng(rng), pokedex(pokedex)
{
}
Battle::Battle(Player &player, std::shared_ptr<NPC> opponent, BattleType type, std::mt19937 &rng, const Pokedex &pokedex)
    : player(player), opponent(opponent), type(type), state(BattleState::intro),
      pendingState(BattleState::choosingAction),
      playerMoveSlot(0), itemChoice(0), switchTarget(0),
      currentAction(BattleAction::fight), rng(rng), pokedex(pokedex)
{
}

void Battle::start()
{
    // Battle already starts in BattleState::intro (from constructor).
    // Phase 0: opponent (Daemon or NPC) slides in.
    introPhase = 0;

    if (opponent == nullptr)
    {
        // Wild: after phase 0 anim, show message then player sends out
        addMessage("A wild " + getOpponentDaemon().getNickname() + " appeared!");
    }
    else
    {
        // Trainer: after phase 0 (NPC appears), show lines, then phase 1 (NPC out, Daemon in)
        addMessage(opponent->getName() + " wants to fight!");
        addMessage(opponent->getName() + " sent out " + getOpponentDaemon().getNickname() + "!");
        addIntroAnimMarker(); // phase 1: NPC slides out, their Daemon comes in
    }

    addMessage("Go " + getPlayerDaemon().getNickname() + "!");
    addIntroAnimMarker(); // next phase: player Daemon comes in

    pendingState = BattleState::choosingAction;
    // state stays as BattleState::intro (set by constructor)
}

void Battle::chooseAction(BattleAction action)
{
    currentAction = action;
    switch (action)
    {
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
        if (canFlee())
        {
            addMessage("Got away safely!");
            pendingState = BattleState::fled;
            state = BattleState::showingMessages;
        }
        else
        {
            addMessage("Can't escape!");
            pendingState = BattleState::choosingAction;
            state = BattleState::showingMessages;
        }
        break;
    }
}

void Battle::chooseMove(int moveSlot)
{
    playerMoveSlot = moveSlot;
    executeTurn();
}

void Battle::chooseItem(int itemId)
{
    itemChoice = itemId;
    const ItemData &item = pokedex.getItem(itemId);

    if (item.category == ItemCategory::healing)
    {
        Daemon &pc = getPlayerDaemon();
        if (pc.getCurrentHP() >= pc.getMaxHP())
        {
            addMessage("HP is already full!");
            pendingState = BattleState::choosingItem;
            state = BattleState::showingMessages;
            return;
        }

        // Remove item from inventory
        player.removeItem(itemId, 1);

        int healBefore = pc.getCurrentHP();
        if (item.effectValue >= 9999)
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
    }
    else if (item.category == ItemCategory::capture)
    {
        // Remove item from inventory
        player.removeItem(itemId, 1);
        attemptCapture(itemId);
    }
    else
    {
        addMessage("Can't use that here!");
        pendingState = BattleState::choosingItem;
        state = BattleState::showingMessages;
    }
}

void Battle::chooseSwitchTarget(int partyIndex)
{
    switchTarget = partyIndex;

    if (partyIndex == 0)
    {
        addMessage("That Daemon is already out!");
        pendingState = BattleState::choosingSwitch;
        state = BattleState::showingMessages;
        return;
    }

    Daemon &target = player.getDaemon(partyIndex);
    if (target.isFainted())
    {
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

void Battle::goBack()
{
    if (state == BattleState::choosingMove ||
        state == BattleState::choosingItem ||
        state == BattleState::choosingSwitch)
    {
        state = BattleState::choosingAction;
    }
}

void Battle::executeTurn()
{
    Daemon &playerDaemon = player.getDaemon(0); // active Daemon

    if (currentAction == BattleAction::fight)
    {
        const auto &moves = playerDaemon.getMoves();
        if (playerMoveSlot < 0 || playerMoveSlot >= 4 || moves[playerMoveSlot].moveId < 0)
        {
            addMessage("No move selected!");
            pendingState = BattleState::choosingAction;
            state = BattleState::showingMessages;
            return;
        }

        if (!playerDaemon.useMove(playerMoveSlot))
        {
            addMessage("No PP left for that move!");
            pendingState = BattleState::choosingAction;
            state = BattleState::showingMessages;
            return;
        }

        // Player attacks
        const MoveData &moveData = pokedex.getMove(moves[playerMoveSlot].moveId);

        // Accuracy check
        if (!accuracyCheck(moveData.accuracy))
        {
            addMessage(playerDaemon.getNickname() + " used " + moveData.name + "!");
            addMessage("But it missed!");
            pendingState = BattleState::opponentTurn;
            state = BattleState::showingMessages;
            return;
        }

        int damage = calculateDamage(playerDaemon, getOpponentDaemon(), moveData);
        getOpponentDaemon().takeDamage(damage);
        addMessage(playerDaemon.getNickname() + " used " + moveData.name + "!");
        addAttackAnimMarker(true);
        addHPAnimMarker();

        // Type effectiveness message
        float eff = getTypeEffectiveness(moveData.type, getOpponentDaemon().getSpecies().primaryType);
        if (getOpponentDaemon().getSpecies().secondaryType != getOpponentDaemon().getSpecies().primaryType)
            eff *= getTypeEffectiveness(moveData.type, getOpponentDaemon().getSpecies().secondaryType);

        if (eff >= 2.0f)
            addMessage("It's super effective!");
        else if (eff > 0.0f && eff <= 0.5f)
            addMessage("It's not very effective...");
        else if (eff == 0.0f)
            addMessage("It has no effect!");

        addMessage("It dealt " + std::to_string(damage) + " damage!");

        if (getOpponentDaemon().isFainted())
        {
            int exp = calculateExpYield(getOpponentDaemon());
            playerDaemon.addExp(exp);
            addMessage("The opposing " + getOpponentDaemon().getNickname() + " fainted!");
            addMessage("Gained " + std::to_string(exp) + " EXP!");
            addEXPAnimMarker();
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

void Battle::executeOpponentTurn()
{
    Daemon &playerDaemon = player.getDaemon(0);

    // Smart AI: pick the best move based on effectiveness and power
    const auto &oppMoves = getOpponentDaemon().getMoves();
    int bestSlot = -1;
    float bestScore = -1.0f;

    for (int i = 0; i < 4; ++i)
    {
        if (oppMoves[i].moveId < 0 || oppMoves[i].currentPP <= 0)
            continue;

        const MoveData &move = pokedex.getMove(oppMoves[i].moveId);
        float score = static_cast<float>(move.power);

        // Factor in type effectiveness
        float eff = getTypeEffectiveness(move.type, playerDaemon.getSpecies().primaryType);
        if (playerDaemon.getSpecies().secondaryType != playerDaemon.getSpecies().primaryType)
            eff *= getTypeEffectiveness(move.type, playerDaemon.getSpecies().secondaryType);
        score *= eff;

        // STAB bonus consideration
        if (move.type == getOpponentDaemon().getSpecies().primaryType ||
            move.type == getOpponentDaemon().getSpecies().secondaryType)
            score *= 1.5f;

        // Status moves get a base score if they have a useful effect
        if (move.power <= 0 && move.statusEffect != StatusEffect::none
            && playerDaemon.getStatus() == StatusEffect::none)
            score = 40.0f; // Give status moves a moderate score

        // Add small random factor (0.85-1.0) to prevent complete predictability
        std::uniform_int_distribution<int> fuzz(85, 100);
        score *= static_cast<float>(fuzz(rng)) / 100.0f;

        if (score > bestScore)
        {
            bestScore = score;
            bestSlot = i;
        }
    }

    // Fallback: use first available move if no good move found
    if (bestSlot < 0)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (oppMoves[i].moveId >= 0)
            {
                bestSlot = i;
                break;
            }
        }
    }

    if (bestSlot < 0) bestSlot = 0;

    int oppMoveId = oppMoves[bestSlot].moveId;
    if (oppMoveId < 0) oppMoveId = 0;
    getOpponentDaemon().useMove(bestSlot);
    const MoveData &oppMoveData = pokedex.getMove(oppMoveId);

    // Accuracy check
    if (!accuracyCheck(oppMoveData.accuracy))
    {
        addMessage("Foe " + getOpponentDaemon().getNickname() + " used " + oppMoveData.name + "!");
        addMessage("But it missed!");
        pendingState = BattleState::choosingAction;
        state = BattleState::showingMessages;
        return;
    }

    int oppDamage = calculateDamage(getOpponentDaemon(), playerDaemon, oppMoveData);
    playerDaemon.takeDamage(oppDamage);
    addMessage("Foe " + getOpponentDaemon().getNickname() + " used " + oppMoveData.name + "!");
    addAttackAnimMarker(false);
    addHPAnimMarker();

    // Type effectiveness for opponent's attack
    float oppEff = getTypeEffectiveness(oppMoveData.type, playerDaemon.getSpecies().primaryType);
    if (playerDaemon.getSpecies().secondaryType != playerDaemon.getSpecies().primaryType)
        oppEff *= getTypeEffectiveness(oppMoveData.type, playerDaemon.getSpecies().secondaryType);

    if (oppEff >= 2.0f)
        addMessage("It's super effective!");
    else if (oppEff > 0.0f && oppEff <= 0.5f)
        addMessage("It's not very effective...");
    else if (oppEff == 0.0f)
        addMessage("It has no effect!");

    addMessage("It dealt " + std::to_string(oppDamage) + " damage!");

    if (playerDaemon.isFainted())
    {
        addMessage(playerDaemon.getNickname() + " fainted!");
        pendingState = BattleState::defeat;
    }
    else
    {
        pendingState = BattleState::choosingAction;
    }
    state = BattleState::showingMessages;
}

BattleState Battle::getState() const { return state; }
BattleState Battle::getPendingState() const { return pendingState; }

BattleResult Battle::getResult() const
{
    BattleResult result{};
    result.playerWon = (state == BattleState::victory || state == BattleState::captured);
    result.playerFled = (state == BattleState::fled);
    result.captured = (state == BattleState::captured);
    result.expGained = 0;
    result.moneyGained = 0;
    return result;
}

Daemon &Battle::getPlayerDaemon()
{
    return player.getDaemon(0);
}

Daemon &Battle::getOpponentDaemon()
{
    if (opponent == nullptr)
        return *opponentDaemon;
    else
        return opponent->getParty()[0];
}

std::shared_ptr<NPC> Battle::getOpponent()
{
    return opponent;
}

BattleType Battle::getType() const { return type; }

bool Battle::canFlee() const
{
    return type == BattleType::wild;
}

bool Battle::attemptCapture(int itemId)
{
    if (opponent != nullptr)
    {
        addMessage("Cannot capture opponent's Daemons!");
        pendingState = BattleState::choosingAction;
        state = BattleState::showingMessages;
        return false;
    }

    // Proper capture algorithm (based on Gen III-IV formula)
    const Daemon &target = getOpponentDaemon();
    const Species &species = target.getSpecies();
    const ItemData &ball = pokedex.getItem(itemId);

    int maxHP = target.getMaxHP();
    int curHP = target.getCurrentHP();
    int catchRate = species.catchRate;          // Species base catch rate (0-255)
    int ballModifier = ball.effectValue;        // 1 = normal, 2 = great, 3 = ultra

    // Status bonus: sleeping/frozen = 2x, paralyzed/poisoned/burned = 1.5x
    float statusBonus = 1.0f;
    if (target.getStatus() == StatusEffect::deadlocked || target.getStatus() == StatusEffect::entangled)
        statusBonus = 2.0f;
    else if (target.getStatus() != StatusEffect::none)
        statusBonus = 1.5f;

    // Modified catch rate: ((3*maxHP - 2*curHP) * catchRate * ballMod) / (3*maxHP) * statusBonus
    float a = ((3.0f * static_cast<float>(maxHP) - 2.0f * static_cast<float>(curHP))
               * static_cast<float>(catchRate) * static_cast<float>(ballModifier))
              / (3.0f * static_cast<float>(maxHP)) * statusBonus;
    a = std::min(a, 255.0f);

    // If a >= 255, guaranteed catch
    if (a >= 255.0f)
    {
        captureShakes = 4;
        captureSuccess = true;
        addMessage("Used " + ball.name + "!");
        addCaptureAnimMarker();
        addMessage("Gotcha! " + target.getNickname() + " was caught!");
        pendingState = BattleState::captured;
        state = BattleState::showingMessages;

        // Add Daemon to player's party
        Daemon caught(species, target.getLevel());
        caught = target; // copy current state
        player.addDaemon(std::move(caught));
        return true;
    }

    // Shake probability: b = 1048560 / sqrt(sqrt(16711680 / a))
    // Each shake succeeds with probability b/65536
    // 4 successful shakes = capture
    float b = 1048560.0f / std::sqrt(std::sqrt(16711680.0f / a));
    int shakeThreshold = static_cast<int>(b);

    std::uniform_int_distribution<int> dist(0, 65535);
    int shakes = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (dist(rng) < shakeThreshold)
            shakes++;
        else
            break;
    }

    if (shakes == 4)
    {
        captureShakes = 4;
        captureSuccess = true;
        addMessage("Used " + ball.name + "!");
        addCaptureAnimMarker();
        addMessage("Gotcha! " + target.getNickname() + " was caught!");
        pendingState = BattleState::captured;
        state = BattleState::showingMessages;

        // Add Daemon to player's party
        player.addDaemon(Daemon(species, target.getLevel(), target.getExp(),
                                    target.getCurrentHP(), target.getNickname(),
                                    target.getStatus(), target.getIVs(), target.getEVs(),
                                    target.getMoves()));
        return true;
    }

    // Show appropriate failure message based on number of shakes
    captureShakes = shakes;
    captureSuccess = false;
    addMessage("Used " + ball.name + "!");
    addCaptureAnimMarker();

    if (shakes == 0)
        addMessage("Oh no! The Daemon broke free immediately!");
    else if (shakes == 1)
        addMessage("The ball shook once... but it broke free!");
    else if (shakes == 2)
        addMessage("The ball shook twice... but it broke free!");
    else
        addMessage("The ball shook three times... So close!");

    // Opponent gets a turn after failed capture
    pendingState = BattleState::opponentTurn;
    state = BattleState::showingMessages;
    return false;
}

const std::string &Battle::getMessage() const
{
    if (!messages.empty())
        return messages.front();
    return emptyMessage;
}

bool Battle::hasMessages() const
{
    return !messages.empty();
}

void Battle::advanceMessage()
{
    if (!messages.empty())
        messages.pop_front();

    if (messages.empty())
    {
        state = pendingState;
        return;
    }

    // Check if the next message is an animation marker
    if (messages.front() == HP_ANIM_MARKER)
    {
        messages.pop_front();
        state = BattleState::animatingHP;
        return;
    }
    if (messages.front() == EXP_ANIM_MARKER)
    {
        messages.pop_front();
        state = BattleState::animatingEXP;
        return;
    }
    if (messages.front() == INTRO_ANIM_MARKER)
    {
        messages.pop_front();
        introPhase++;
        state = BattleState::intro;
        return;
    }
    if (messages.front() == CAPTURE_ANIM_MARKER)
    {
        messages.pop_front();
        state = BattleState::animatingCapture;
        return;
    }
    if (messages.front() == ATTACK_ANIM_PLAYER_MARKER)
    {
        messages.pop_front();
        attackAnimIsPlayer = true;
        state = BattleState::animatingAttack;
        return;
    }
    if (messages.front() == ATTACK_ANIM_OPP_MARKER)
    {
        messages.pop_front();
        attackAnimIsPlayer = false;
        state = BattleState::animatingAttack;
        return;
    }
}

void Battle::finishIntroAnimation()
{
    if (messages.empty())
    {
        state = pendingState;
        introComplete = true;
    }
    else
        state = BattleState::showingMessages;
}

int Battle::getIntroPhase() const
{
    return introPhase;
}

bool Battle::isIntroComplete() const
{
    return introComplete;
}

void Battle::finishHPAnimation()
{
    if (messages.empty())
        state = pendingState;
    else
        state = BattleState::showingMessages;
}

void Battle::finishEXPAnimation()
{
    if (messages.empty())
        state = pendingState;
    else
        state = BattleState::showingMessages;
}

void Battle::finishCaptureAnimation()
{
    if (messages.empty())
        state = pendingState;
    else
        state = BattleState::showingMessages;
}

int Battle::getCaptureShakes() const
{
    return captureShakes;
}

bool Battle::getCaptureSuccess() const
{
    return captureSuccess;
}

void Battle::addLevelUpMessage(const std::string &msg)
{
    // Insert level-up message + EXP anim marker at front of queue
    // so we show the message, then resume EXP animation for remaining EXP
    messages.push_front(EXP_ANIM_MARKER);
    messages.push_front(msg);
    state = BattleState::showingMessages;
}

void Battle::addMessage(const std::string &msg)
{
    messages.push_back(msg);
}

void Battle::addHPAnimMarker()
{
    messages.push_back(HP_ANIM_MARKER);
}

void Battle::addEXPAnimMarker()
{
    messages.push_back(EXP_ANIM_MARKER);
}

void Battle::addIntroAnimMarker()
{
    messages.push_back(INTRO_ANIM_MARKER);
}

void Battle::addCaptureAnimMarker()
{
    messages.push_back(CAPTURE_ANIM_MARKER);
}

void Battle::addAttackAnimMarker(bool isPlayer)
{
    messages.push_back(isPlayer ? ATTACK_ANIM_PLAYER_MARKER : ATTACK_ANIM_OPP_MARKER);
}

void Battle::finishAttackAnimation()
{
    if (messages.empty())
    {
        state = pendingState;
        return;
    }

    // Check for markers at the front of the queue (same logic as advanceMessage)
    if (messages.front() == HP_ANIM_MARKER)
    {
        messages.pop_front();
        state = BattleState::animatingHP;
        return;
    }
    if (messages.front() == EXP_ANIM_MARKER)
    {
        messages.pop_front();
        state = BattleState::animatingEXP;
        return;
    }
    state = BattleState::showingMessages;
}

bool Battle::isPlayerAttacking() const
{
    return attackAnimIsPlayer;
}

// --- Private helpers ---

int Battle::calculateDamage(const Daemon &attacker, const Daemon &defender,
                            const MoveData &move) const
{
    if (move.power <= 0)
        return 0;

    // Use physical or special stats based on move category
    int attack, defense;
    if (move.category == MoveCategory::special)
    {
        attack = attacker.getStat(3);  // special attack
        defense = defender.getStat(4); // special defense
    }
    else
    {
        attack = attacker.getStat(1);  // attack
        defense = defender.getStat(2); // defense
    }

    int level = attacker.getLevel();
    int power = move.power;

    // Standard Pokémon damage formula
    int baseDamage = ((2 * level / 5 + 2) * power * attack / defense / 50) + 2;

    // Type effectiveness
    float eff = getTypeEffectiveness(move.type, defender.getSpecies().primaryType);
    if (defender.getSpecies().secondaryType != defender.getSpecies().primaryType)
        eff *= getTypeEffectiveness(move.type, defender.getSpecies().secondaryType);
    baseDamage = static_cast<int>(static_cast<float>(baseDamage) * eff);

    // STAB (Same Type Attack Bonus)
    if (move.type == attacker.getSpecies().primaryType ||
        move.type == attacker.getSpecies().secondaryType)
    {
        baseDamage = baseDamage * 3 / 2;
    }

    // Random factor 85-100%
    std::uniform_int_distribution<int> dist(85, 100);
    int roll = dist(rng);
    baseDamage = baseDamage * roll / 100;

    return std::max(1, baseDamage);
}

float Battle::getTypeEffectiveness(ElementType attackType, ElementType defenseType) const
{
    return TypeChart::getEffectiveness(attackType, defenseType);
}

bool Battle::accuracyCheck(int accuracy) const
{
    std::uniform_int_distribution<int> dist(1, 100);
    return dist(rng) <= accuracy;
}

int Battle::calculateExpYield(const Daemon &defeated) const
{
    return defeated.getSpecies().baseExpYield * defeated.getLevel() / 7;
}
