#include "BattleTurnResolver.h"
#include "../game_data/Pokedex.h"
#include "../state/Daemon.h"
#include "BattleAI.h"
#include "BattleRules.h"
#include <array>

namespace {

constexpr int moveSlotCount = 4;
constexpr int damageRollMinPercent = 85;
constexpr int damageRollMaxPercent = 100;

} // namespace

BattleMoveSelection BattleTurnResolver::preparePlayerMove(Daemon &playerDaemon, int moveSlot,
                                                          const Pokedex &pokedex) {
    const auto &moves = playerDaemon.getMoves();
    if (moveSlot < 0 || moveSlot >= moveSlotCount ||
        moves[static_cast<std::size_t>(moveSlot)].moveId < 0) {
        return BattleMoveSelection{BattleMoveSelectionError::invalidSelection, moveSlot, nullptr};
    }

    if (!playerDaemon.useMove(moveSlot))
        return BattleMoveSelection{BattleMoveSelectionError::noPP, moveSlot, nullptr};

    const MoveData &move = pokedex.getMove(moves[static_cast<std::size_t>(moveSlot)].moveId);
    return BattleMoveSelection{BattleMoveSelectionError::none, moveSlot, &move};
}

BattleMoveSelection BattleTurnResolver::prepareOpponentMove(Daemon &opponentDaemon,
                                                            const Daemon &playerDaemon,
                                                            const Pokedex &pokedex,
                                                            std::mt19937 &rng) {
    const auto &opponentMoves = opponentDaemon.getMoves();
    std::array<BattleAI::MoveCandidate, moveSlotCount> candidates{};
    for (int i = 0; i < moveSlotCount; ++i) {
        const MoveSlot &slot = opponentMoves[static_cast<std::size_t>(i)];
        if (slot.moveId >= 0)
            candidates[static_cast<std::size_t>(i)] =
                BattleAI::MoveCandidate{&pokedex.getMove(slot.moveId), slot.currentPP};
    }

    const int slot =
        BattleAI::chooseMoveSlot(candidates, opponentDaemon.getSpecies(), playerDaemon, rng);
    if (slot < 0 || slot >= moveSlotCount ||
        opponentMoves[static_cast<std::size_t>(slot)].moveId < 0) {
        return BattleMoveSelection{BattleMoveSelectionError::invalidSelection, slot, nullptr};
    }

    if (!opponentDaemon.useMove(slot))
        return BattleMoveSelection{BattleMoveSelectionError::noPP, slot, nullptr};

    const MoveData &move = pokedex.getMove(opponentMoves[static_cast<std::size_t>(slot)].moveId);
    return BattleMoveSelection{BattleMoveSelectionError::none, slot, &move};
}

BattleAttackResolution BattleTurnResolver::resolveAttack(Daemon &attacker, Daemon &defender,
                                                         const MoveData &move, std::mt19937 &rng) {
    std::uniform_int_distribution<int> accuracyRoll(1, 100);
    if (accuracyRoll(rng) > move.accuracy)
        return BattleAttackResolution{};

    std::uniform_int_distribution<int> damageRoll(damageRollMinPercent, damageRollMaxPercent);
    const int damage = BattleRules::calculateDamage(attacker, defender, move, damageRoll(rng));
    defender.takeDamage(damage);

    const float effectiveness =
        BattleRules::effectivenessMultiplier(move.type, defender.getSpecies());
    return BattleAttackResolution{true, damage, effectiveness, defender.isFainted()};
}
