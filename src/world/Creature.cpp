#include "Creature.h"
#include <algorithm>

Creature::Creature(const Species &species, int level)
    : speciesRef(&species), speciesId(species.id), level(level), exp(0), currentHP(0), status(StatusEffect::none),
      ivs{0, 0, 0, 0, 0, 0}, evs{0, 0, 0, 0, 0, 0}
{
    nickname = species.name;

    // Init moves to empty
    for (auto &moveSlot : moves)
    {
        moveSlot.moveId = -1;
        moveSlot.currentPP = 0;
        moveSlot.maxPP = 0;
    }

    std::vector<LearnableMove> available;
    for (const auto &learnable : species.learnset)
    {
        if (learnable.levelLearned <= level)
            available.push_back(learnable);
    }
    int slot = 0;
    int start = std::max(0, static_cast<int>(available.size()) - 4);
    for (int i = start; i < static_cast<int>(available.size()) && slot < 4; ++i, ++slot)
    {
        moves[slot] = {available[i].moveId, 15, 15};
    }

    currentHP = calculateStat(species.baseStats.hp, ivs.hp, evs.hp, true);
}

Creature::Creature(const Species &species, int level, int exp, int currentHP,
                   const std::string &nickname, StatusEffect status,
                   const BaseStats &ivs, const BaseStats &evs,
                   const std::array<MoveSlot, 4> &moves)
    : speciesRef(&species), speciesId(species.id), level(level), exp(exp),
      currentHP(currentHP), status(status), ivs(ivs), evs(evs), moves(moves)
{
    this->nickname = nickname;
}

int Creature::calculateStat(int base, int iv, int ev, bool isHP) const
{
    if (isHP)
    {
        return ((2 * base + iv + ev / 4) * level / 100) + level + 10;
    }
    return ((2 * base + iv + ev / 4) * level / 100) + 5;
}

int Creature::getMaxHP() const
{
    return calculateStat(speciesRef->baseStats.hp, ivs.hp, evs.hp, true);
}

int Creature::getCurrentHP() const { return currentHP; }

void Creature::takeDamage(int amount)
{
    currentHP = std::max(0, currentHP - amount);
}

void Creature::heal(int amount)
{
    currentHP = std::min(getMaxHP(), currentHP + amount);
}

void Creature::fullHeal()
{
    currentHP = getMaxHP();
    status = StatusEffect::none;
}

bool Creature::isFainted() const { return currentHP <= 0; }

const std::string &Creature::getNickname() const { return nickname; }
void Creature::setNickname(const std::string &name) { nickname = name; }
int Creature::getLevel() const { return level; }
int Creature::getExp() const { return exp; }
const Species &Creature::getSpecies() const { return *speciesRef; }
int Creature::getSpeciesId() const { return speciesId; }
const BaseStats &Creature::getIVs() const { return ivs; }
const BaseStats &Creature::getEVs() const { return evs; }
StatusEffect Creature::getStatus() const { return status; }
void Creature::setStatus(StatusEffect s) { status = s; }
void Creature::clearStatus() { status = StatusEffect::none; }
const std::array<MoveSlot, 4> &Creature::getMoves() const { return moves; }

bool Creature::useMove(int slot)
{
    if (slot < 0 || slot >= 4 || moves[slot].moveId < 0)
        return false;
    if (moves[slot].currentPP <= 0)
        return false;
    moves[slot].currentPP--;
    return true;
}

void Creature::addExp(int amount) { exp += amount; }

int Creature::getExpNeeded() const
{
    return level * level * level;
}

bool Creature::checkLevelUp()
{
    int needed = getExpNeeded();
    if (exp >= needed)
    {
        exp -= needed;
        level++;
        int oldMax = getMaxHP();
        int newMax = calculateStat(speciesRef->baseStats.hp, ivs.hp, evs.hp, true);
        currentHP += (newMax - oldMax);
        return true;
    }
    return false;
}

int Creature::getStat(int statIndex) const
{
    const BaseStats &bs = speciesRef->baseStats;
    switch (statIndex)
    {
    case 0:
        return getMaxHP();
    case 1:
        return calculateStat(bs.attack, ivs.attack, evs.attack, false);
    case 2:
        return calculateStat(bs.defense, ivs.defense, evs.defense, false);
    case 3:
        return calculateStat(bs.specialAttack, ivs.specialAttack, evs.specialAttack, false);
    case 4:
        return calculateStat(bs.specialDefense, ivs.specialDefense, evs.specialDefense, false);
    case 5:
        return calculateStat(bs.speed, ivs.speed, evs.speed, false);
    default:
        return 0;
    }
}

bool Creature::learnMove(int moveId, int slot)
{
    if (slot < 0 || slot >= 4)
        return false;
    moves[slot] = {moveId, 15, 15}; // PP should come from MoveData lookup
    return true;
}