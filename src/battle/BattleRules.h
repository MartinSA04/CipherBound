#pragma once

#include "BattleTypes.h"
#include "../game_data/Move.h"
#include "../game_data/Species.h"
#include "../state/Daemon.h"

namespace BattleRules {

float effectivenessMultiplier(ElementType attackType, const Species &defenderSpecies);

int calculateDamage(const Daemon &attacker, const Daemon &defender, const MoveData &move,
                    int randomPercent);

int calculateExpYield(const Daemon &defeated);
int calculateMoneyReward(const Daemon &defeated, BattleType type);

} // namespace BattleRules
