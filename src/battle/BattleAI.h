#pragma once

#include "../data/Move.h"
#include "../data/Species.h"
#include "../state/Daemon.h"
#include <random>
#include <span>

namespace BattleAI {

struct MoveCandidate {
    const MoveData *move{nullptr};
    int currentPP{0};
};

float scoreMove(const MoveData &move, const Species &attackerSpecies, const Daemon &defender,
                float randomFactor = 1.0f);

int chooseMoveSlot(std::span<const MoveCandidate> candidates, const Species &attackerSpecies,
                   const Daemon &defender, std::mt19937 &rng);

} // namespace BattleAI
