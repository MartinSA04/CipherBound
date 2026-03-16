#include "BattleAI.h"
#include "TypeChart.h"

namespace {

constexpr float stabMultiplier = 1.5f;
constexpr float statusMoveBaseScore = 40.0f;
constexpr int moveScoreFuzzMinPercent = 85;
constexpr int moveScoreFuzzMaxPercent = 100;

float effectivenessAgainst(const MoveData &move, const Daemon &defender) {
    const Species &species = defender.getSpecies();
    float effectiveness = TypeChart::getEffectiveness(move.type, species.primaryType);
    if (species.secondaryType != species.primaryType)
        effectiveness *= TypeChart::getEffectiveness(move.type, species.secondaryType);
    return effectiveness;
}

bool getsStab(const MoveData &move, const Species &attackerSpecies) {
    return move.type == attackerSpecies.primaryType || move.type == attackerSpecies.secondaryType;
}

} // namespace

namespace BattleAI {

float scoreMove(const MoveData &move, const Species &attackerSpecies, const Daemon &defender,
                float randomFactor) {
    float score = static_cast<float>(move.power);
    score *= effectivenessAgainst(move, defender);

    if (getsStab(move, attackerSpecies))
        score *= stabMultiplier;

    if (move.power <= 0 && move.statusEffect != StatusEffect::none &&
        defender.getStatus() == StatusEffect::none) {
        score = statusMoveBaseScore;
    }

    return score * randomFactor;
}

int chooseMoveSlot(std::span<const MoveCandidate> candidates, const Species &attackerSpecies,
                   const Daemon &defender, std::mt19937 &rng) {
    int bestSlot = -1;
    float bestScore = -1.0f;

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const MoveCandidate &candidate = candidates[i];
        if (candidate.move == nullptr || candidate.currentPP <= 0)
            continue;

        std::uniform_int_distribution<int> fuzz(moveScoreFuzzMinPercent, moveScoreFuzzMaxPercent);
        float randomFactor = static_cast<float>(fuzz(rng)) / 100.0f;
        float score = scoreMove(*candidate.move, attackerSpecies, defender, randomFactor);
        if (score > bestScore) {
            bestScore = score;
            bestSlot = static_cast<int>(i);
        }
    }

    if (bestSlot >= 0)
        return bestSlot;

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (candidates[i].move != nullptr)
            return static_cast<int>(i);
    }

    return 0;
}

} // namespace BattleAI
