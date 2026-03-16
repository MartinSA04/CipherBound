#pragma once

#include "../data/Move.h"
#include "../data/Species.h"
#include "../state/Daemon.h"

namespace BattleRules {

float effectivenessMultiplier(ElementType attackType, const Species &defenderSpecies);

int calculateDamage(const Daemon &attacker, const Daemon &defender, const MoveData &move,
                    int randomPercent);

int calculateExpYield(const Daemon &defeated);

} // namespace BattleRules
