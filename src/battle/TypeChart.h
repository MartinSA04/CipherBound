#pragma once
#include "../game_data/Species.h"

// Damage and type effectiveness calculations
namespace TypeChart {
float getEffectiveness(ElementType attackType, ElementType defenseType);
float getEffectiveness(ElementType attackType, ElementType primaryDef, ElementType secondaryDef);
bool isImmune(ElementType attackType, ElementType defenseType);
bool isSuperEffective(ElementType attackType, ElementType defenseType);
bool isNotVeryEffective(ElementType attackType, ElementType defenseType);
} // namespace TypeChart
