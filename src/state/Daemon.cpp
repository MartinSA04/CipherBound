#include "Daemon.h"
#include <algorithm>
#include <numeric>

namespace {

constexpr int maxDaemonLevel = 100;
constexpr int maxIVValue = 31;
constexpr int maxEVPerStat = 255;
constexpr int maxTotalEVs = 510;

int clampLevel(int level) { return std::clamp(level, 1, maxDaemonLevel); }

std::array<int, 6> toArray(const BaseStats &stats) {
    return {stats.hp,   stats.attack, stats.defense, stats.specialAttack, stats.specialDefense,
            stats.speed};
}

BaseStats fromArray(const std::array<int, 6> &values) {
    return {values[0], values[1], values[2], values[3], values[4], values[5]};
}

BaseStats clampStats(const BaseStats &stats, int minValue, int maxValue) {
    auto values = toArray(stats);
    for (int &value : values)
        value = std::clamp(value, minValue, maxValue);
    return fromArray(values);
}

BaseStats normalizeEVs(const BaseStats &stats) {
    const auto clamped = toArray(clampStats(stats, 0, maxEVPerStat));
    const int total = std::accumulate(clamped.begin(), clamped.end(), 0);
    if (total <= maxTotalEVs)
        return fromArray(clamped);

    std::array<int, 6> scaled{};
    std::array<int, 6> remainders{};
    int used = 0;
    for (std::size_t i = 0; i < clamped.size(); ++i) {
        scaled[i] = clamped[i] * maxTotalEVs / total;
        remainders[i] = clamped[i] * maxTotalEVs % total;
        used += scaled[i];
    }

    int remaining = maxTotalEVs - used;
    while (remaining > 0) {
        std::size_t bestIndex = 0;
        for (std::size_t i = 1; i < remainders.size(); ++i) {
            if (remainders[i] > remainders[bestIndex])
                bestIndex = i;
        }
        scaled[bestIndex]++;
        remainders[bestIndex] = -1;
        remaining--;
    }

    return fromArray(scaled);
}

int totalStats(const BaseStats &stats) {
    const auto values = toArray(stats);
    return std::accumulate(values.begin(), values.end(), 0);
}

enum class NatureAffectedStat { none, attack, defense, speed, specialAttack, specialDefense };

NatureAffectedStat increasedStatFor(Nature nature) {
    switch (nature) {
    case Nature::lonely:
    case Nature::brave:
    case Nature::adamant:
    case Nature::naughty:
        return NatureAffectedStat::attack;
    case Nature::bold:
    case Nature::relaxed:
    case Nature::impish:
    case Nature::lax:
        return NatureAffectedStat::defense;
    case Nature::timid:
    case Nature::hasty:
    case Nature::jolly:
    case Nature::naive:
        return NatureAffectedStat::speed;
    case Nature::modest:
    case Nature::mild:
    case Nature::quiet:
    case Nature::rash:
        return NatureAffectedStat::specialAttack;
    case Nature::calm:
    case Nature::gentle:
    case Nature::sassy:
    case Nature::careful:
        return NatureAffectedStat::specialDefense;
    default:
        return NatureAffectedStat::none;
    }
}

NatureAffectedStat decreasedStatFor(Nature nature) {
    switch (nature) {
    case Nature::bold:
    case Nature::timid:
    case Nature::modest:
    case Nature::calm:
        return NatureAffectedStat::attack;
    case Nature::lonely:
    case Nature::hasty:
    case Nature::mild:
    case Nature::gentle:
        return NatureAffectedStat::defense;
    case Nature::brave:
    case Nature::relaxed:
    case Nature::quiet:
    case Nature::sassy:
        return NatureAffectedStat::speed;
    case Nature::adamant:
    case Nature::impish:
    case Nature::jolly:
    case Nature::careful:
        return NatureAffectedStat::specialAttack;
    case Nature::naughty:
    case Nature::lax:
    case Nature::naive:
    case Nature::rash:
        return NatureAffectedStat::specialDefense;
    default:
        return NatureAffectedStat::none;
    }
}

NatureAffectedStat natureStatForIndex(int statIndex) {
    switch (statIndex) {
    case 1:
        return NatureAffectedStat::attack;
    case 2:
        return NatureAffectedStat::defense;
    case 3:
        return NatureAffectedStat::specialAttack;
    case 4:
        return NatureAffectedStat::specialDefense;
    case 5:
        return NatureAffectedStat::speed;
    default:
        return NatureAffectedStat::none;
    }
}

int applyNatureModifier(int value, Nature nature, NatureAffectedStat stat) {
    if (stat == NatureAffectedStat::none)
        return value;

    const NatureAffectedStat increased = increasedStatFor(nature);
    const NatureAffectedStat decreased = decreasedStatFor(nature);
    if (stat == increased && stat != decreased)
        return value * 11 / 10;
    if (stat == decreased && stat != increased)
        return value * 9 / 10;
    return value;
}

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
    : speciesId(species.id), level(clampLevel(level)), exp(totalExpForCurrentLevel(species, level)),
      currentHP(0), status(StatusEffect::none), ivs{0, 0, 0, 0, 0, 0}, evs{0, 0, 0, 0, 0, 0},
      nature(Nature::hardy), speciesRef(species) {
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

    currentHP = calculateStat(species.baseStats.hp, ivs.hp, evs.hp, 0);
}

Daemon Daemon::generateRandomized(const Species &species, int level, std::mt19937 &rng) {
    std::uniform_int_distribution<int> ivRoll(0, maxIVValue);
    std::uniform_int_distribution<int> natureRoll(static_cast<int>(Nature::hardy),
                                                  static_cast<int>(Nature::quirky));

    Daemon daemon(species, level);
    daemon.ivs = {ivRoll(rng), ivRoll(rng), ivRoll(rng), ivRoll(rng), ivRoll(rng), ivRoll(rng)};
    daemon.evs = {0, 0, 0, 0, 0, 0};
    daemon.nature = static_cast<Nature>(natureRoll(rng));
    daemon.currentHP = daemon.getMaxHP();
    return daemon;
}

Daemon::Daemon(const Species &species, int level, int exp, int currentHP,
               const std::string &nickname, StatusEffect status, const BaseStats &ivs,
               const BaseStats &evs, const std::array<MoveSlot, 4> &moves, Nature nature)
    : speciesId(species.id), level(clampLevel(level)), exp(normalizeTotalExp(species, level, exp)),
      currentHP(currentHP), status(status), ivs(clampStats(ivs, 0, maxIVValue)),
      evs(normalizeEVs(evs)), nature(nature), moves(moves), speciesRef(species) {
    this->level = levelFromTotalExp(species, this->exp);
    this->currentHP = std::clamp(this->currentHP, 0, getMaxHP());
    this->nickname = nickname;
}

int Daemon::calculateStatAtLevel(int base, int iv, int ev, int statLevel, int statIndex) const {
    if (statIndex == 0) {
        return ((2 * base + iv + ev / 4) * statLevel / 100) + statLevel + 10;
    }
    const int rawStat = ((2 * base + iv + ev / 4) * statLevel / 100) + 5;
    return applyNatureModifier(rawStat, nature, natureStatForIndex(statIndex));
}

int Daemon::calculateStat(int base, int iv, int ev, int statIndex) const {
    return calculateStatAtLevel(base, iv, ev, level, statIndex);
}

BaseStats Daemon::calculateStatsForLevel(int statLevel) const {
    const BaseStats &baseStats = speciesRef.get().baseStats;
    return {calculateStatAtLevel(baseStats.hp, ivs.hp, evs.hp, statLevel, 0),
            calculateStatAtLevel(baseStats.attack, ivs.attack, evs.attack, statLevel, 1),
            calculateStatAtLevel(baseStats.defense, ivs.defense, evs.defense, statLevel, 2),
            calculateStatAtLevel(baseStats.specialAttack, ivs.specialAttack, evs.specialAttack,
                                 statLevel, 3),
            calculateStatAtLevel(baseStats.specialDefense, ivs.specialDefense, evs.specialDefense,
                                 statLevel, 4),
            calculateStatAtLevel(baseStats.speed, ivs.speed, evs.speed, statLevel, 5)};
}

int Daemon::getMaxHP() const {
    return calculateStat(speciesRef.get().baseStats.hp, ivs.hp, evs.hp, 0);
}

int Daemon::getCurrentHP() const { return currentHP; }

void Daemon::takeDamage(int amount) { currentHP = std::max(0, currentHP - amount); }

void Daemon::heal(int amount) { currentHP = std::min(getMaxHP(), currentHP + amount); }

void Daemon::fullHeal() {
    currentHP = getMaxHP();
    status = StatusEffect::none;
    for (MoveSlot &moveSlot : moves) {
        if (moveSlot.moveId >= 0)
            moveSlot.currentPP = moveSlot.maxPP;
    }
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
Nature Daemon::getNature() const { return nature; }
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

BaseStats Daemon::gainEffortValues(const BaseStats &yield) {
    const int oldMaxHP = getMaxHP();

    auto currentValues = toArray(evs);
    const auto yieldValues = toArray(clampStats(yield, 0, maxEVPerStat));
    std::array<int, 6> applied{};
    int total = totalStats(evs);

    for (std::size_t i = 0; i < currentValues.size(); ++i) {
        const int perStatRoom = maxEVPerStat - currentValues[i];
        const int totalRoom = maxTotalEVs - total;
        const int gained = std::clamp(yieldValues[i], 0, std::min(perStatRoom, totalRoom));
        currentValues[i] += gained;
        applied[i] = gained;
        total += gained;
    }

    evs = fromArray(currentValues);

    const int newMaxHP = getMaxHP();
    currentHP = std::clamp(currentHP + (newMaxHP - oldMaxHP), 0, newMaxHP);
    return fromArray(applied);
}

int Daemon::getExpNeeded() const {
    if (level >= maxDaemonLevel)
        return 1;
    return totalExpForNextLevel(speciesRef.get(), level) -
           totalExpForCurrentLevel(speciesRef.get(), level);
}

bool Daemon::checkLevelUp() { return resolveLevelUp().has_value(); }

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

    return LevelUpResult{
        oldLevel,
        level,
        {newStats.hp - oldStats.hp, newStats.attack - oldStats.attack,
         newStats.defense - oldStats.defense, newStats.specialAttack - oldStats.specialAttack,
         newStats.specialDefense - oldStats.specialDefense, newStats.speed - oldStats.speed}};
}

std::vector<int> Daemon::getMovesLearnedAtLevel(int learnedLevel) const {
    std::vector<int> learnedMoves;
    for (const LearnableMove &learnable : speciesRef.get().learnset) {
        if (learnable.levelLearned == learnedLevel)
            learnedMoves.push_back(learnable.moveId);
    }
    return learnedMoves;
}

std::optional<int> Daemon::getEvolutionTargetSpeciesId() const {
    for (const EvolutionInfo &evolution : speciesRef.get().evolutions) {
        if (level >= evolution.levelRequired)
            return evolution.targetSpeciesId;
    }
    return std::nullopt;
}

void Daemon::evolveTo(const Species &species) {
    const Species &oldSpecies = speciesRef.get();
    const bool hadDefaultNickname = nickname == oldSpecies.name;
    const int oldMaxHP = getMaxHP();

    speciesId = species.id;
    speciesRef = species;

    const int newMaxHP = getMaxHP();
    currentHP = std::clamp(currentHP + (newMaxHP - oldMaxHP), 0, newMaxHP);

    if (hadDefaultNickname)
        nickname = species.name;
}

int Daemon::getStat(int statIndex) const {
    const BaseStats &bs = speciesRef.get().baseStats;
    switch (statIndex) {
    case 0:
        return getMaxHP();
    case 1:
        return calculateStat(bs.attack, ivs.attack, evs.attack, 1);
    case 2:
        return calculateStat(bs.defense, ivs.defense, evs.defense, 2);
    case 3:
        return calculateStat(bs.specialAttack, ivs.specialAttack, evs.specialAttack, 3);
    case 4:
        return calculateStat(bs.specialDefense, ivs.specialDefense, evs.specialDefense, 4);
    case 5:
        return calculateStat(bs.speed, ivs.speed, evs.speed, 5);
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
