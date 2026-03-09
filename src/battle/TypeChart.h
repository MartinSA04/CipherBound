#pragma once
#include "../data/Species.h"

// Damage and type effectiveness calculations
namespace TypeChart
{
    // Returns the type effectiveness multiplier (0.0, 0.5, 1.0, 2.0)
    float getEffectiveness(ElementType attackType, ElementType defenseType);

    // Returns combined effectiveness against dual-type
    float getEffectiveness(ElementType attackType, ElementType primaryDef, ElementType secondaryDef);

    // Returns true if the attack type has no effect
    bool isImmune(ElementType attackType, ElementType defenseType);

    // Returns true if the attack is super effective (>1.0)
    bool isSuperEffective(ElementType attackType, ElementType defenseType);

    // Returns true if the attack is not very effective (<1.0)
    bool isNotVeryEffective(ElementType attackType, ElementType defenseType);
}
