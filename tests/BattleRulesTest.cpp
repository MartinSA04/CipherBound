#include "../src/battle/BattleRules.h"
#include "../src/game_data/Move.h"
#include "../src/game_data/Species.h"
#include "../src/state/Daemon.h"
#include <cassert>
#include <string>

namespace {

Species makeSpecies(int id, const std::string &name, ElementType primary, ElementType secondary) {
    Species species{};
    species.id = id;
    species.name = name;
    species.primaryType = primary;
    species.secondaryType = secondary;
    species.baseStats = {50, 30, 25, 30, 25, 20};
    species.catchRate = 120;
    species.baseExpYield = 70;
    return species;
}

MoveData makeMove(int id, const std::string &name, ElementType type, MoveCategory category,
                  int power) {
    MoveData move{};
    move.id = id;
    move.name = name;
    move.description = "";
    move.type = type;
    move.category = category;
    move.power = power;
    move.accuracy = 100;
    move.maxPP = 15;
    move.statusEffect = StatusEffect::none;
    move.statusChance = 0;
    return move;
}

} // namespace

int main() {
    const Species attackerSpecies =
        makeSpecies(1, "Attacker", ElementType::quantum, ElementType::logic);
    const Species defenderSpecies =
        makeSpecies(2, "Defender", ElementType::classical, ElementType::classical);

    const Daemon attacker(attackerSpecies, 20);
    const Daemon defender(defenderSpecies, 20);

    const MoveData stabSpecialMove =
        makeMove(1, "Quantum Beam", ElementType::quantum, MoveCategory::special, 60);
    const MoveData neutralSpecialMove =
        makeMove(2, "Digital Beam", ElementType::digital, MoveCategory::special, 60);
    const MoveData statusMove = makeMove(3, "Status", ElementType::logic, MoveCategory::status, 0);

    assert(BattleRules::effectivenessMultiplier(ElementType::quantum, defenderSpecies) == 2.0f);
    assert(BattleRules::effectivenessMultiplier(ElementType::logic, defenderSpecies) == 1.0f);

    assert(BattleRules::calculateDamage(attacker, defender, statusMove, 100) == 0);

    const int stabDamage = BattleRules::calculateDamage(attacker, defender, stabSpecialMove, 100);
    const int neutralDamage =
        BattleRules::calculateDamage(attacker, defender, neutralSpecialMove, 100);
    assert(stabDamage > neutralDamage);

    const int rolledLow = BattleRules::calculateDamage(attacker, defender, neutralSpecialMove, 85);
    const int rolledHigh =
        BattleRules::calculateDamage(attacker, defender, neutralSpecialMove, 100);
    assert(rolledHigh >= rolledLow);

    assert(BattleRules::calculateExpYield(defender) == defenderSpecies.baseExpYield * 20 / 7);

    return 0;
}
