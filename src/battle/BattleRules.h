/**
 * @file
 * @brief Core battle formulas shared by turn resolution and tests.
 * @ingroup battle_system
 */

#pragma once

#include "BattleTypes.h"
#include "../game_data/Move.h"
#include "../game_data/Species.h"
#include "../state/Daemon.h"

/**
 * @brief Deterministic battle rule calculations.
 * @ingroup battle_system
 */
namespace BattleRules {

/// Returns the elemental effectiveness multiplier against the defender species.
float effectivenessMultiplier(ElementType attackType, const Species &defenderSpecies);

/// Calculates move damage using the current attacker, defender, and random roll.
int calculateDamage(const Daemon &attacker, const Daemon &defender, const MoveData &move,
                    int randomPercent);

/// Calculates experience yield for a defeated daemon.
int calculateExpYield(const Daemon &defeated);
/// Calculates post-battle money reward for the given battle type.
int calculateMoneyReward(const Daemon &defeated, BattleType type);

} // namespace BattleRules
