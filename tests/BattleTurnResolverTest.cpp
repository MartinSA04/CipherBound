#include "../src/battle/BattleTurnResolver.h"
#include "../src/game_data/Move.h"
#include "../src/game_data/Species.h"
#include "../src/state/Daemon.h"
#include <cassert>
#include <random>
#include <string>

namespace {

Species makeSpecies(int id, const std::string &name) {
    Species species{};
    species.id = id;
    species.name = name;
    species.primaryType = ElementType::classical;
    species.secondaryType = ElementType::logic;
    species.baseStats = {40, 18, 18, 18, 18, 18};
    species.catchRate = 120;
    species.baseExpYield = 64;
    return species;
}

MoveData makeMove(int accuracy) {
    return MoveData{1, "Proof Strike", "", ElementType::classical, MoveCategory::physical, 50,
                    accuracy, 20, StatusEffect::none, 0};
}

} // namespace

int main() {
    const Species attackerSpecies = makeSpecies(1, "Alpha");
    const Species defenderSpecies = makeSpecies(2, "Beta");
    Daemon attacker(attackerSpecies, 10);
    Daemon defender(defenderSpecies, 10);

    std::mt19937 rng(123);

    const int hpBeforeMiss = defender.getCurrentHP();
    const BattleAttackResolution miss =
        BattleTurnResolver::resolveAttack(attacker, defender, makeMove(0), rng);
    assert(!miss.hit);
    assert(defender.getCurrentHP() == hpBeforeMiss);

    const int hpBeforeHit = defender.getCurrentHP();
    const BattleAttackResolution hit =
        BattleTurnResolver::resolveAttack(attacker, defender, makeMove(100), rng);
    assert(hit.hit);
    assert(hit.damage > 0);
    assert(defender.getCurrentHP() == hpBeforeHit - hit.damage);

    return 0;
}
