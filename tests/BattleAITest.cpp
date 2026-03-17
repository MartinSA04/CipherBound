#include "../src/battle/BattleAI.h"
#include "../src/game_data/Move.h"
#include "../src/game_data/Species.h"
#include "../src/state/Daemon.h"
#include <array>
#include <cassert>
#include <random>
#include <string>

namespace {

Species makeSpecies(int id, const std::string &name, ElementType primary, ElementType secondary) {
    Species species{};
    species.id = id;
    species.name = name;
    species.primaryType = primary;
    species.secondaryType = secondary;
    species.baseStats = {40, 20, 20, 20, 20, 20};
    species.catchRate = 120;
    species.baseExpYield = 64;
    return species;
}

MoveData makeMove(int id, const std::string &name, ElementType type, int power,
                  StatusEffect statusEffect = StatusEffect::none) {
    MoveData move{};
    move.id = id;
    move.name = name;
    move.description = "";
    move.type = type;
    move.category = power > 0 ? MoveCategory::special : MoveCategory::status;
    move.power = power;
    move.accuracy = 100;
    move.maxPP = 15;
    move.statusEffect = statusEffect;
    move.statusChance = statusEffect == StatusEffect::none ? 0 : 100;
    return move;
}

} // namespace

int main() {
    const Species attacker = makeSpecies(1, "Attacker", ElementType::quantum, ElementType::logic);
    const Species defenderSpecies =
        makeSpecies(2, "Defender", ElementType::classical, ElementType::classical);
    Daemon defender(defenderSpecies, 10);

    const MoveData neutralMove = makeMove(1, "Neutral", ElementType::digital, 60);
    const MoveData superEffectiveMove = makeMove(2, "Quantum Hit", ElementType::quantum, 60);
    const MoveData stabMove = makeMove(3, "Quantum Pulse", ElementType::quantum, 50);
    const MoveData nonStabMove = makeMove(4, "Digital Pulse", ElementType::digital, 50);
    const MoveData statusMove =
        makeMove(5, "Lockdown", ElementType::logic, 0, StatusEffect::deadlocked);

    assert(BattleAI::scoreMove(superEffectiveMove, attacker, defender) >
           BattleAI::scoreMove(neutralMove, attacker, defender));
    assert(BattleAI::scoreMove(stabMove, attacker, defender) >
           BattleAI::scoreMove(nonStabMove, attacker, defender));

    float statusScore = BattleAI::scoreMove(statusMove, attacker, defender);
    assert(statusScore == 40.0f);
    defender.setStatus(StatusEffect::paradox);
    assert(BattleAI::scoreMove(statusMove, attacker, defender) == 0.0f);
    defender.clearStatus();

    const MoveData depletedBestMove = makeMove(6, "Depleted", ElementType::quantum, 120);
    const MoveData usableMove = makeMove(7, "Usable", ElementType::digital, 50);
    const MoveData strongestUsableMove = makeMove(8, "Strongest", ElementType::quantum, 80);

    const std::array<BattleAI::MoveCandidate, 4> candidates = {
        BattleAI::MoveCandidate{&depletedBestMove, 0},
        BattleAI::MoveCandidate{&usableMove, 10},
        BattleAI::MoveCandidate{&strongestUsableMove, 10},
        BattleAI::MoveCandidate{nullptr, 0},
    };

    std::mt19937 rng(123);
    assert(BattleAI::chooseMoveSlot(candidates, attacker, defender, rng) == 2);

    return 0;
}
