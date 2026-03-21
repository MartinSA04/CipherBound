#include "../src/game_data/Species.h"
#include "../src/state/Daemon.h"
#include <array>
#include <cassert>

namespace {

Species makeSpecies(int id, GrowthRate growthRate) {
    Species species{};
    species.id = id;
    species.name = "Testmon";
    species.primaryType = ElementType::classical;
    species.secondaryType = ElementType::classical;
    species.growthRate = growthRate;
    species.baseStats = {45, 49, 49, 65, 65, 45};
    species.catchRate = 45;
    species.baseExpYield = 64;
    return species;
}

std::array<MoveSlot, 4> emptyMoves() {
    return {{{-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}}};
}

std::array<MoveSlot, 4> fullMoves() {
    return {{{1, 35, 35}, {2, 35, 35}, {3, 35, 35}, {4, 35, 35}}};
}

} // namespace

int main() {
    assert(Daemon(makeSpecies(1, GrowthRate::fast), 50).getExp() == 100000);
    assert(Daemon(makeSpecies(2, GrowthRate::mediumFast), 50).getExp() == 125000);
    assert(Daemon(makeSpecies(3, GrowthRate::mediumSlow), 50).getExp() == 117360);
    assert(Daemon(makeSpecies(4, GrowthRate::slow), 50).getExp() == 156250);
    assert(Daemon(makeSpecies(5, GrowthRate::erratic), 100).getExp() == 600000);
    assert(Daemon(makeSpecies(6, GrowthRate::fluctuating), 100).getExp() == 1640000);

    Daemon mediumSlow(makeSpecies(7, GrowthRate::mediumSlow), 5);
    assert(mediumSlow.getExpProgress() == 0);
    assert(mediumSlow.getExpNeeded() == 44);

    Daemon legacyProgress(makeSpecies(8, GrowthRate::mediumFast), 5, 10, 20, "Legacy",
                          StatusEffect::none, {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0},
                          emptyMoves());
    assert(legacyProgress.getLevel() == 5);
    assert(legacyProgress.getExp() == 135);
    assert(legacyProgress.getExpProgress() == 10);

    Daemon leveler(makeSpecies(9, GrowthRate::mediumFast), 5);
    leveler.addExp(90);
    assert(!leveler.checkLevelUp());
    leveler.addExp(1);
    assert(leveler.checkLevelUp());
    assert(leveler.getLevel() == 6);
    assert(leveler.getExp() == 216);
    assert(leveler.getExpProgress() == 0);

    Species learner = makeSpecies(10, GrowthRate::mediumFast);
    learner.learnset = {{7, 6}, {8, 7}};

    Daemon progression(learner, 5);
    progression.addExp(91);
    const auto firstLevel = progression.resolveLevelUp();
    assert(firstLevel.has_value());
    assert(firstLevel->oldLevel == 5);
    assert(firstLevel->newLevel == 6);
    assert(firstLevel->statGains.hp > 0);
    assert(firstLevel->statGains.attack > 0);
    const std::vector<int> levelSixMoves = progression.getMovesLearnedAtLevel(6);
    assert(levelSixMoves.size() == 1);
    assert(levelSixMoves[0] == 7);

    Daemon moveSlots(learner, 5, 125, 20, "Moves", StatusEffect::none, {0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0}, emptyMoves());
    assert(moveSlots.firstEmptyMoveSlot() == 0);
    assert(!moveSlots.knowsMove(7));
    assert(moveSlots.learnMove(7, 0, 20));
    assert(moveSlots.knowsMove(7));

    Daemon fullMoveSet(learner, 5, 125, 20, "Full", StatusEffect::none, {0, 0, 0, 0, 0, 0},
                       {0, 0, 0, 0, 0, 0}, fullMoves());
    assert(fullMoveSet.firstEmptyMoveSlot() == -1);
    assert(fullMoveSet.knowsMove(1));

    return 0;
}
