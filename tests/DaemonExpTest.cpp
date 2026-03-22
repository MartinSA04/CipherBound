#include "../src/game_data/Species.h"
#include "../src/state/Daemon.h"
#include <array>
#include <cassert>
#include <random>

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
    learner.evolutions = {{11, 6}};
    Species evolved = makeSpecies(11, GrowthRate::mediumFast);
    evolved.name = "Nextmon";
    evolved.baseStats = {60, 70, 65, 80, 75, 60};

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
    assert(progression.getEvolutionTargetSpeciesId().has_value());
    assert(*progression.getEvolutionTargetSpeciesId() == 11);
    progression.evolveTo(evolved);
    assert(progression.getSpeciesId() == 11);
    assert(progression.getSpecies().name == "Nextmon");
    assert(progression.getNickname() == "Nextmon");
    assert(progression.getMaxHP() > 0);

    Daemon nicknamed(learner, 6, 216, 20, "Buddy", StatusEffect::none, {0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0}, emptyMoves());
    nicknamed.evolveTo(evolved);
    assert(nicknamed.getNickname() == "Buddy");

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

    Species statSpecies = makeSpecies(12, GrowthRate::mediumFast);
    const int level50Exp = Daemon(statSpecies, 50).getExp();
    Daemon adamant(statSpecies, 50, level50Exp, 120, "Adamant", StatusEffect::none,
                   {31, 31, 31, 31, 31, 31}, {0, 252, 0, 0, 0, 4}, emptyMoves(),
                   Nature::adamant);
    assert(adamant.getMaxHP() == 120);
    assert(adamant.getStat(1) == 111);
    assert(adamant.getStat(2) == 69);
    assert(adamant.getStat(3) == 76);
    assert(adamant.getStat(4) == 85);
    assert(adamant.getStat(5) == 66);

    Daemon normalized(statSpecies, 50, level50Exp, 120, "Normalized", StatusEffect::none,
                      {99, -5, 31, 40, 12, 77}, {255, 255, 255, 255, 255, 255}, emptyMoves(),
                      Nature::jolly);
    assert(normalized.getIVs().hp == 31);
    assert(normalized.getIVs().attack == 0);
    assert(normalized.getIVs().defense == 31);
    assert(normalized.getIVs().specialAttack == 31);
    assert(normalized.getIVs().specialDefense == 12);
    assert(normalized.getIVs().speed == 31);
    assert(normalized.getEVs().hp == 85);
    assert(normalized.getEVs().attack == 85);
    assert(normalized.getEVs().defense == 85);
    assert(normalized.getEVs().specialAttack == 85);
    assert(normalized.getEVs().specialDefense == 85);
    assert(normalized.getEVs().speed == 85);
    assert(normalized.getNature() == Nature::jolly);

    std::mt19937 rngA(1337);
    std::mt19937 rngB(1337);
    Daemon generatedA = Daemon::generateRandomized(statSpecies, 8, rngA);
    Daemon generatedB = Daemon::generateRandomized(statSpecies, 8, rngB);
    assert(generatedA.getLevel() == 8);
    assert(generatedA.getNature() == generatedB.getNature());
    assert(generatedA.getIVs().hp == generatedB.getIVs().hp);
    assert(generatedA.getIVs().attack == generatedB.getIVs().attack);
    assert(generatedA.getIVs().defense == generatedB.getIVs().defense);
    assert(generatedA.getIVs().specialAttack == generatedB.getIVs().specialAttack);
    assert(generatedA.getIVs().specialDefense == generatedB.getIVs().specialDefense);
    assert(generatedA.getIVs().speed == generatedB.getIVs().speed);
    assert(generatedA.getEVs().hp == 0);
    assert(generatedA.getEVs().attack == 0);
    assert(generatedA.getEVs().defense == 0);
    assert(generatedA.getEVs().specialAttack == 0);
    assert(generatedA.getEVs().specialDefense == 0);
    assert(generatedA.getEVs().speed == 0);
    assert(generatedA.getCurrentHP() == generatedA.getMaxHP());
    assert(generatedA.getIVs().hp >= 0 && generatedA.getIVs().hp <= 31);
    assert(generatedA.getIVs().attack >= 0 && generatedA.getIVs().attack <= 31);
    assert(generatedA.getIVs().defense >= 0 && generatedA.getIVs().defense <= 31);
    assert(generatedA.getIVs().specialAttack >= 0 && generatedA.getIVs().specialAttack <= 31);
    assert(generatedA.getIVs().specialDefense >= 0 && generatedA.getIVs().specialDefense <= 31);
    assert(generatedA.getIVs().speed >= 0 && generatedA.getIVs().speed <= 31);
    assert(generatedA.getNature() != Nature::hardy || generatedA.getIVs().hp != 0 ||
           generatedA.getIVs().attack != 0 || generatedA.getIVs().defense != 0 ||
           generatedA.getIVs().specialAttack != 0 || generatedA.getIVs().specialDefense != 0 ||
           generatedA.getIVs().speed != 0);

    Daemon evGain(statSpecies, 50, level50Exp, 120, "EVGain", StatusEffect::none,
                  {31, 31, 31, 31, 31, 31}, {0, 0, 0, 0, 0, 0}, emptyMoves(), Nature::hardy);
    const BaseStats applied = evGain.gainEffortValues({4, 8, 0, 0, 0, 0});
    assert(applied.hp == 4);
    assert(applied.attack == 8);
    assert(evGain.getEVs().hp == 4);
    assert(evGain.getEVs().attack == 8);
    assert(evGain.getMaxHP() == 121);
    assert(evGain.getCurrentHP() == 121);
    assert(evGain.getStat(1) == 70);

    Daemon cappedEVs(statSpecies, 50, level50Exp, 120, "Capped", StatusEffect::none,
                     {31, 31, 31, 31, 31, 31}, {252, 252, 0, 0, 0, 0}, emptyMoves(),
                     Nature::hardy);
    const BaseStats cappedApplied = cappedEVs.gainEffortValues({8, 8, 8, 8, 8, 8});
    assert(cappedApplied.hp == 3);
    assert(cappedApplied.attack == 3);
    assert(cappedApplied.defense == 0);
    assert(cappedEVs.getEVs().hp == 255);
    assert(cappedEVs.getEVs().attack == 255);
    assert(cappedEVs.getEVs().defense == 0);

    return 0;
}
