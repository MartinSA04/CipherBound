/**
 * @file
 * @brief Heuristics for choosing opponent moves.
 * @ingroup battle_system
 */

#pragma once

#include "../game_data/Move.h"
#include "../game_data/Species.h"
#include "../state/Daemon.h"
#include <random>
#include <span>

/**
 * @brief Simple battle AI helpers for opponent turn selection.
 * @ingroup battle_system
 */
namespace BattleAI {

/// Candidate move with the PP state needed for selection.
struct MoveCandidate {
    const MoveData *move{nullptr}; ///< Referenced move data, if available.
    int currentPP{0};              ///< Remaining PP for the move slot.
};

/// Scores one move against the current defender for weighted selection.
float scoreMove(const MoveData &move, const Species &attackerSpecies, const Daemon &defender,
                float randomFactor = 1.0f);

/// Chooses a move slot index from the available candidates.
int chooseMoveSlot(std::span<const MoveCandidate> candidates, const Species &attackerSpecies,
                   const Daemon &defender, std::mt19937 &rng);

} // namespace BattleAI
