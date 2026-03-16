#include "Daemon.h"
#include <algorithm>

Daemon::Daemon(const Species &species, int level)
    : speciesId(species.id), level(level), exp(0), currentHP(0), status(StatusEffect::none), ivs{0, 0, 0, 0, 0, 0},
      evs{0, 0, 0, 0, 0, 0}, speciesRef(&species) {
    nickname = species.name;

    // Init moves to empty
    for (auto &moveSlot : moves) {
        moveSlot.moveId = -1;
        moveSlot.currentPP = 0;
        moveSlot.maxPP = 0;
    }

    std::vector<LearnableMove> available;
    for (const auto &learnable : species.learnset) {
        if (learnable.levelLearned <= level)
            available.push_back(learnable);
    }
    int slot = 0;
    int availableCount = static_cast<int>(available.size());
    int start = (availableCount > 4) ? (availableCount - 4) : 0;
    for (int i = start; i < availableCount && slot < 4; ++i, ++slot) {
        moves[static_cast<std::size_t>(slot)] = {available[static_cast<std::size_t>(i)].moveId, 15, 15};
    }

    currentHP = calculateStat(species.baseStats.hp, ivs.hp, evs.hp, true);
}

Daemon::Daemon(const Species &species, int level, int exp, int currentHP, const std::string &nickname,
               StatusEffect status, const BaseStats &ivs, const BaseStats &evs, const std::array<MoveSlot, 4> &moves)
    : speciesId(species.id), level(level), exp(exp), currentHP(currentHP), status(status), ivs(ivs), evs(evs),
      moves(moves), speciesRef(&species) {
    this->nickname = nickname;
}

int Daemon::calculateStat(int base, int iv, int ev, bool isHP) const {
    if (isHP) {
        return ((2 * base + iv + ev / 4) * level / 100) + level + 10;
    }
    return ((2 * base + iv + ev / 4) * level / 100) + 5;
}

int Daemon::getMaxHP() const { return calculateStat(speciesRef->baseStats.hp, ivs.hp, evs.hp, true); }

int Daemon::getCurrentHP() const { return currentHP; }

void Daemon::takeDamage(int amount) { currentHP = std::max(0, currentHP - amount); }

void Daemon::heal(int amount) { currentHP = std::min(getMaxHP(), currentHP + amount); }

void Daemon::fullHeal() {
    currentHP = getMaxHP();
    status = StatusEffect::none;
}

bool Daemon::isFainted() const { return currentHP <= 0; }

const std::string &Daemon::getNickname() const { return nickname; }
void Daemon::setNickname(const std::string &name) { nickname = name; }
int Daemon::getLevel() const { return level; }
int Daemon::getExp() const { return exp; }
const Species &Daemon::getSpecies() const { return *speciesRef; }
int Daemon::getSpeciesId() const { return speciesId; }
const BaseStats &Daemon::getIVs() const { return ivs; }
const BaseStats &Daemon::getEVs() const { return evs; }
StatusEffect Daemon::getStatus() const { return status; }
void Daemon::setStatus(StatusEffect s) { status = s; }
void Daemon::clearStatus() { status = StatusEffect::none; }
const std::array<MoveSlot, 4> &Daemon::getMoves() const { return moves; }

bool Daemon::useMove(int slot) {
    if (slot < 0 || slot >= 4 || moves[static_cast<std::size_t>(slot)].moveId < 0)
        return false;
    if (moves[static_cast<std::size_t>(slot)].currentPP <= 0)
        return false;
    moves[static_cast<std::size_t>(slot)].currentPP--;
    return true;
}

void Daemon::addExp(int amount) { exp += amount; }

int Daemon::getExpNeeded() const { return level * level * level; }

bool Daemon::checkLevelUp() {
    int needed = getExpNeeded();
    if (exp >= needed) {
        exp -= needed;
        level++;
        int oldMax = getMaxHP();
        int newMax = calculateStat(speciesRef->baseStats.hp, ivs.hp, evs.hp, true);
        currentHP += (newMax - oldMax);
        return true;
    }
    return false;
}

int Daemon::getStat(int statIndex) const {
    const BaseStats &bs = speciesRef->baseStats;
    switch (statIndex) {
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

bool Daemon::learnMove(int moveId, int slot) {
    if (slot < 0 || slot >= 4)
        return false;
    moves[static_cast<std::size_t>(slot)] = {moveId, 15, 15}; // PP should come from MoveData lookup
    return true;
}