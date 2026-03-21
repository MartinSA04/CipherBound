#include "Daemon.h"
#include <algorithm>

namespace {

constexpr int maxDaemonLevel = 100;

int clampLevel(int level) { return std::clamp(level, 1, maxDaemonLevel); }

int totalExpForLevel(GrowthRate growthRate, int level) {
    const int n = clampLevel(level);
    const int n2 = n * n;
    const int n3 = n2 * n;

    int value = 0;
    switch (growthRate) {
    case GrowthRate::erratic:
        if (n <= 50)
            value = n3 * (100 - n) / 50;
        else if (n <= 68)
            value = n3 * (150 - n) / 100;
        else if (n <= 98)
            value = n3 * ((1911 - 10 * n) / 3) / 500;
        else
            value = n3 * (160 - n) / 100;
        break;
    case GrowthRate::fast:
        value = 4 * n3 / 5;
        break;
    case GrowthRate::mediumFast:
        value = n3;
        break;
    case GrowthRate::mediumSlow:
        value = 6 * n3 / 5 - 15 * n2 + 100 * n - 140;
        break;
    case GrowthRate::slow:
        value = 5 * n3 / 4;
        break;
    case GrowthRate::fluctuating:
        if (n <= 15)
            value = n3 * ((n + 1) / 3 + 24) / 50;
        else if (n <= 36)
            value = n3 * (n + 14) / 50;
        else
            value = n3 * (n / 2 + 32) / 50;
        break;
    }

    return std::max(0, value);
}

int totalExpForCurrentLevel(const Species &species, int level) {
    return totalExpForLevel(species.growthRate, level);
}

int totalExpForNextLevel(const Species &species, int level) {
    if (level >= maxDaemonLevel)
        return totalExpForCurrentLevel(species, maxDaemonLevel);
    return totalExpForLevel(species.growthRate, level + 1);
}

int levelFromTotalExp(const Species &species, int totalExp) {
    int level = 1;
    for (int nextLevel = 2; nextLevel <= maxDaemonLevel; ++nextLevel) {
        if (totalExp < totalExpForLevel(species.growthRate, nextLevel))
            break;
        level = nextLevel;
    }
    return level;
}

int normalizeTotalExp(const Species &species, int level, int savedExp) {
    const int clampedLevel = clampLevel(level);
    int totalExp = std::max(0, savedExp);
    const int levelFloor = totalExpForCurrentLevel(species, clampedLevel);

    // Old saves stored EXP inside the current level instead of cumulative EXP.
    if (totalExp < levelFloor)
        totalExp += levelFloor;

    return totalExp;
}

} // namespace

Daemon::Daemon(const Species &species, int level)
    : speciesId(species.id), level(clampLevel(level)),
      exp(totalExpForCurrentLevel(species, level)), currentHP(0), status(StatusEffect::none),
      ivs{0, 0, 0, 0, 0, 0}, evs{0, 0, 0, 0, 0, 0}, speciesRef(species) {
    nickname = species.name;

    // Init moves to empty
    for (auto &moveSlot : moves) {
        moveSlot.moveId = -1;
        moveSlot.currentPP = 0;
        moveSlot.maxPP = 0;
    }

    std::vector<LearnableMove> available;
    for (const auto &learnable : species.learnset) {
        if (learnable.levelLearned <= this->level)
            available.push_back(learnable);
    }
    int slot = 0;
    int availableCount = static_cast<int>(available.size());
    int start = (availableCount > 4) ? (availableCount - 4) : 0;
    for (int i = start; i < availableCount && slot < 4; ++i, ++slot) {
        moves[static_cast<std::size_t>(slot)] = {available[static_cast<std::size_t>(i)].moveId, 15,
                                                 15};
    }

    currentHP = calculateStat(species.baseStats.hp, ivs.hp, evs.hp, true);
}

Daemon::Daemon(const Species &species, int level, int exp, int currentHP,
               const std::string &nickname, StatusEffect status, const BaseStats &ivs,
               const BaseStats &evs, const std::array<MoveSlot, 4> &moves)
    : speciesId(species.id), level(clampLevel(level)),
      exp(normalizeTotalExp(species, level, exp)), currentHP(currentHP), status(status), ivs(ivs),
      evs(evs), moves(moves), speciesRef(species) {
    this->level = levelFromTotalExp(species, this->exp);
    this->currentHP = std::clamp(this->currentHP, 0, getMaxHP());
    this->nickname = nickname;
}

int Daemon::calculateStatAtLevel(int base, int iv, int ev, int statLevel, bool isHP) const {
    if (isHP) {
        return ((2 * base + iv + ev / 4) * statLevel / 100) + statLevel + 10;
    }
    return ((2 * base + iv + ev / 4) * statLevel / 100) + 5;
}

int Daemon::calculateStat(int base, int iv, int ev, bool isHP) const {
    return calculateStatAtLevel(base, iv, ev, level, isHP);
}

BaseStats Daemon::calculateStatsForLevel(int statLevel) const {
    const BaseStats &baseStats = speciesRef.get().baseStats;
    return {calculateStatAtLevel(baseStats.hp, ivs.hp, evs.hp, statLevel, true),
            calculateStatAtLevel(baseStats.attack, ivs.attack, evs.attack, statLevel, false),
            calculateStatAtLevel(baseStats.defense, ivs.defense, evs.defense, statLevel, false),
            calculateStatAtLevel(baseStats.specialAttack, ivs.specialAttack, evs.specialAttack,
                                 statLevel, false),
            calculateStatAtLevel(baseStats.specialDefense, ivs.specialDefense, evs.specialDefense,
                                 statLevel, false),
            calculateStatAtLevel(baseStats.speed, ivs.speed, evs.speed, statLevel, false)};
}

int Daemon::getMaxHP() const {
    return calculateStat(speciesRef.get().baseStats.hp, ivs.hp, evs.hp, true);
}

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
int Daemon::getExpProgress() const {
    if (level >= maxDaemonLevel)
        return getExpNeeded();
    return exp - totalExpForCurrentLevel(speciesRef.get(), level);
}
const Species &Daemon::getSpecies() const { return speciesRef.get(); }
int Daemon::getSpeciesId() const { return speciesId; }
const BaseStats &Daemon::getIVs() const { return ivs; }
const BaseStats &Daemon::getEVs() const { return evs; }
StatusEffect Daemon::getStatus() const { return status; }
void Daemon::setStatus(StatusEffect s) { status = s; }
void Daemon::clearStatus() { status = StatusEffect::none; }
const std::array<MoveSlot, 4> &Daemon::getMoves() const { return moves; }

bool Daemon::knowsMove(int moveId) const {
    return std::any_of(moves.begin(), moves.end(),
                       [moveId](const MoveSlot &slot) { return slot.moveId == moveId; });
}

int Daemon::firstEmptyMoveSlot() const {
    for (int i = 0; i < 4; ++i) {
        if (moves[static_cast<std::size_t>(i)].moveId < 0)
            return i;
    }
    return -1;
}

bool Daemon::useMove(int slot) {
    if (slot < 0 || slot >= 4 || moves[static_cast<std::size_t>(slot)].moveId < 0)
        return false;
    if (moves[static_cast<std::size_t>(slot)].currentPP <= 0)
        return false;
    moves[static_cast<std::size_t>(slot)].currentPP--;
    return true;
}

void Daemon::addExp(int amount) { exp += amount; }

int Daemon::getExpNeeded() const {
    if (level >= maxDaemonLevel)
        return 1;
    return totalExpForNextLevel(speciesRef.get(), level) -
           totalExpForCurrentLevel(speciesRef.get(), level);
}

bool Daemon::checkLevelUp() {
    return resolveLevelUp().has_value();
}

std::optional<LevelUpResult> Daemon::resolveLevelUp() {
    if (level >= maxDaemonLevel)
        return std::nullopt;

    if (exp < totalExpForNextLevel(speciesRef.get(), level))
        return std::nullopt;

    const int oldLevel = level;
    const BaseStats oldStats = calculateStatsForLevel(oldLevel);

    level++;

    const BaseStats newStats = calculateStatsForLevel(level);
    currentHP += (newStats.hp - oldStats.hp);

    return LevelUpResult{oldLevel,
                         level,
                         {newStats.hp - oldStats.hp,
                          newStats.attack - oldStats.attack,
                          newStats.defense - oldStats.defense,
                          newStats.specialAttack - oldStats.specialAttack,
                          newStats.specialDefense - oldStats.specialDefense,
                          newStats.speed - oldStats.speed}};
}

std::vector<int> Daemon::getMovesLearnedAtLevel(int learnedLevel) const {
    std::vector<int> learnedMoves;
    for (const LearnableMove &learnable : speciesRef.get().learnset) {
        if (learnable.levelLearned == learnedLevel)
            learnedMoves.push_back(learnable.moveId);
    }
    return learnedMoves;
}

int Daemon::getStat(int statIndex) const {
    const BaseStats &bs = speciesRef.get().baseStats;
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

bool Daemon::learnMove(int moveId, int slot, int maxPP) {
    if (slot < 0 || slot >= 4)
        return false;
    moves[static_cast<std::size_t>(slot)] = {moveId, maxPP, maxPP};
    return true;
}
