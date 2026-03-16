#include "TypeChart.h"
#include <array>

namespace TypeChart {
// Number of types in ElementType enum
static constexpr int NUM_TYPES = 15;

// Type order: classical, electromagnetic, quantum, nuclear, radioactive,
// thermal, gravitational, algebraic, calculus, probabilistic, algorithmic,
// logic, digital, recursive, chaotic
//
// Effectiveness rationale:
//   Quantum beats Classical (QM supersedes Newton)
//   EM beats Digital (EMP fries circuits)
//   Nuclear beats Thermal (fusion > heat)
//   Logic beats Recursive (halting problem / termination proofs)
//   Chaotic resists Algorithmic (unpredictable systems)
//   Gravitational beats Classical (GR supersedes Newton)
//   Calculus beats Algebraic (analysis generalizes algebra)
//   Probabilistic beats Quantum (measurement collapses states)
//   Recursive is immune to Logic (Gödel's incompleteness)

static constexpr std::array<std::array<int, NUM_TYPES>, NUM_TYPES> chart = {{
    //                      CLA  EM  QUA NUC RAD THE GRA ALG CAL PRO ALG LOG DIG
    //                      REC CHA
    /* Classical      */ {{2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2}},
    /* Electromagnetic */ {{2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2}},
    /* Quantum         */ {{4, 2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 4, 2}},
    /* Nuclear         */ {{2, 2, 2, 1, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2}},
    /* Radioactive     */ {{2, 2, 4, 1, 1, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2}},
    /* Thermal         */ {{2, 2, 2, 1, 2, 1, 2, 2, 2, 2, 4, 2, 4, 2, 2}},
    /* Gravitational   */ {{4, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 4}},
    /* Algebraic       */ {{2, 2, 2, 2, 2, 2, 2, 1, 1, 4, 2, 4, 2, 2, 2}},
    /* Calculus        */ {{2, 2, 2, 2, 2, 2, 4, 4, 1, 2, 2, 2, 2, 2, 1}},
    /* Probabilistic   */ {{2, 2, 4, 2, 2, 2, 2, 1, 2, 1, 2, 1, 2, 2, 4}},
    /* Algorithmic     */ {{2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 1, 4, 4, 1, 1}},
    /* Logic           */ {{2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 1, 1, 2, 0, 2}},
    /* Digital         */ {{2, 1, 2, 2, 1, 1, 2, 2, 2, 2, 1, 4, 2, 4, 2}},
    /* Recursive       */ {{2, 2, 1, 2, 2, 2, 2, 2, 4, 2, 4, 2, 2, 1, 2}},
    /* Chaotic         */ {{2, 2, 2, 2, 2, 2, 1, 2, 4, 1, 4, 2, 2, 4, 1}},
}};

static float chartValueToMultiplier(int value) {
    switch (value) {
    case 0:
        return 0.0f;
    case 1:
        return 0.5f;
    case 4:
        return 2.0f;
    default:
        return 1.0f;
    }
}

static std::size_t typeToIndex(ElementType t) { return static_cast<std::size_t>(t); }

float getEffectiveness(ElementType attackType, ElementType defenseType) {
    std::size_t atk = typeToIndex(attackType);
    std::size_t def = typeToIndex(defenseType);
    if (atk >= NUM_TYPES || def >= NUM_TYPES)
        return 1.0f;
    return chartValueToMultiplier(chart[atk][def]);
}

float getEffectiveness(ElementType attackType, ElementType primaryDef, ElementType secondaryDef) {
    return getEffectiveness(attackType, primaryDef) * getEffectiveness(attackType, secondaryDef);
}

bool isImmune(ElementType attackType, ElementType defenseType) {
    return getEffectiveness(attackType, defenseType) == 0.0f;
}

bool isSuperEffective(ElementType attackType, ElementType defenseType) {
    return getEffectiveness(attackType, defenseType) > 1.0f;
}

bool isNotVeryEffective(ElementType attackType, ElementType defenseType) {
    float eff = getEffectiveness(attackType, defenseType);
    return eff > 0.0f && eff < 1.0f;
}
} // namespace TypeChart
