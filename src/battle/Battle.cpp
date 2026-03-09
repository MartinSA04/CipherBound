#include "Battle.h"
#include "TypeChart.h"
#include <algorithm>

Battle::Battle(Player &player, std::unique_ptr<Creature> opponent, BattleType type, std::mt19937 &rng, const Pokedex &pokedex)
    : player(player), opponentCreature(std::move(opponent)), type(type), state(BattleState::intro),
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
    // Phase 0: opponent (creature or NPC) slides in.
    introPhase = 0;

    if (opponent == nullptr)
    {
        // Wild: after phase 0 anim, show message then player sends out
        addMessage("A wild " + getOpponentCreature().getNickname() + " appeared!");
    }
    else
    {
        // Trainer: after phase 0 (NPC appears), show lines, then phase 1 (NPC out, creature in)
        addMessage(opponent->getName() + " wants to fight!");
        addMessage(opponent->getName() + " sent out " + getOpponentCreature().getNickname() + "!");
        addIntroAnimMarker(); // phase 1: NPC slides out, their creature comes in
    }

    addMessage("Go " + getPlayerCreature().getNickname() + "!");
    addIntroAnimMarker(); // next phase: player creature comes in

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
    case BattleAction::switchCreature:
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
        Creature &pc = getPlayerCreature();
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
        addMessage("That creature is already out!");
        pendingState = BattleState::choosingSwitch;
        state = BattleState::showingMessages;
        return;
    }

    Creature &target = player.getCreature(partyIndex);
    if (target.isFainted())
    {
        addMessage(target.getNickname() + " has no energy left!");
        pendingState = BattleState::choosingSwitch;
        state = BattleState::showingMessages;
        return;
    }

    // Swap the creature to the front
    std::string oldName = player.getCreature(0).getNickname();
    player.swapCreature(0, partyIndex);
    addMessage("Come back, " + oldName + "!");
    addMessage("Go, " + player.getCreature(0).getNickname() + "!");

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
    Creature &playerCreature = player.getCreature(0); // active creature

    if (currentAction == BattleAction::fight)
    {
        const auto &moves = playerCreature.getMoves();
        if (playerMoveSlot < 0 || playerMoveSlot >= 4 || moves[playerMoveSlot].moveId < 0)
        {
            addMessage("No move selected!");
            pendingState = BattleState::choosingAction;
            state = BattleState::showingMessages;
            return;
        }

        if (!playerCreature.useMove(playerMoveSlot))
        {
            addMessage("No PP left for that move!");
            pendingState = BattleState::choosingAction;
            state = BattleState::showingMessages;
            return;
        }

        // Player attacks
        const MoveData &moveData = pokedex.getMove(moves[playerMoveSlot].moveId);
        int damage = calculateDamage(playerCreature, getOpponentCreature(), moveData);
        getOpponentCreature().takeDamage(damage);
        addMessage(playerCreature.getNickname() + " used " + moveData.name + "!");
        addHPAnimMarker();

        // Type effectiveness message
        float eff = getTypeEffectiveness(moveData.type, getOpponentCreature().getSpecies().primaryType);
        if (getOpponentCreature().getSpecies().secondaryType != getOpponentCreature().getSpecies().primaryType)
            eff *= getTypeEffectiveness(moveData.type, getOpponentCreature().getSpecies().secondaryType);

        if (eff >= 2.0f)
            addMessage("It's super effective!");
        else if (eff > 0.0f && eff <= 0.5f)
            addMessage("It's not very effective...");
        else if (eff == 0.0f)
            addMessage("It has no effect!");

        addMessage("It dealt " + std::to_string(damage) + " damage!");

        if (getOpponentCreature().isFainted())
        {
            int exp = calculateExpYield(getOpponentCreature());
            playerCreature.addExp(exp);
            addMessage("The opposing " + getOpponentCreature().getNickname() + " fainted!");
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
    Creature &playerCreature = player.getCreature(0);

    // Opponent attacks (simple AI: always use first move)
    const auto &oppMoves = getOpponentCreature().getMoves();
    int oppMoveId = (oppMoves[0].moveId >= 0) ? oppMoves[0].moveId : 0;
    getOpponentCreature().useMove(0);
    const MoveData &oppMoveData = pokedex.getMove(oppMoveId);
    int oppDamage = calculateDamage(getOpponentCreature(), playerCreature, oppMoveData);
    playerCreature.takeDamage(oppDamage);
    addMessage("Foe " + getOpponentCreature().getNickname() + " used " + oppMoveData.name + "!");
    addHPAnimMarker();

    // Type effectiveness for opponent's attack
    float oppEff = getTypeEffectiveness(oppMoveData.type, playerCreature.getSpecies().primaryType);
    if (playerCreature.getSpecies().secondaryType != playerCreature.getSpecies().primaryType)
        oppEff *= getTypeEffectiveness(oppMoveData.type, playerCreature.getSpecies().secondaryType);

    if (oppEff >= 2.0f)
        addMessage("It's super effective!");
    else if (oppEff > 0.0f && oppEff <= 0.5f)
        addMessage("It's not very effective...");
    else if (oppEff == 0.0f)
        addMessage("It has no effect!");

    addMessage("It dealt " + std::to_string(oppDamage) + " damage!");

    if (playerCreature.isFainted())
    {
        addMessage(playerCreature.getNickname() + " fainted!");
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

Creature &Battle::getPlayerCreature()
{
    return player.getCreature(0);
}

Creature &Battle::getOpponentCreature()
{
    if (opponent == nullptr)
        return *opponentCreature;
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
        addMessage("Cannot capture opponent's creatures!");
        pendingState = BattleState::choosingAction;
        state = BattleState::showingMessages;
        return false;
    }
    (void)itemId;
    // Simplified capture: random chance
    std::uniform_int_distribution<int> dist(0, 99);
    int roll = dist(rng);

    float hpRatio = static_cast<float>(getOpponentCreature().getCurrentHP()) / static_cast<float>(getOpponentCreature().getMaxHP());
    int threshold = static_cast<int>(60.0f * (1.0f - hpRatio)) + 10;

    if (roll < threshold)
    {
        addMessage("Gotcha! " + getOpponentCreature().getNickname() + " was caught!");
        pendingState = BattleState::captured;
        state = BattleState::showingMessages;
        return true;
    }

    addMessage("Oh no! It broke free!");
    pendingState = BattleState::choosingAction;
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

// --- Private helpers ---

int Battle::calculateDamage(const Creature &attacker, const Creature &defender,
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
    baseDamage = static_cast<int>(baseDamage * eff);

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

int Battle::calculateExpYield(const Creature &defeated) const
{
    return defeated.getSpecies().baseExpYield * defeated.getLevel() / 7;
}
