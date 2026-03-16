#include "BattleRules.h"
#include "TypeChart.h"
#include <algorithm>

namespace BattleRules {

float effectivenessMultiplier(ElementType attackType, const Species &defenderSpecies) {
    float effectiveness = TypeChart::getEffectiveness(attackType, defenderSpecies.primaryType);
    if (defenderSpecies.secondaryType != defenderSpecies.primaryType)
        effectiveness *= TypeChart::getEffectiveness(attackType, defenderSpecies.secondaryType);
    return effectiveness;
}

int calculateDamage(const Daemon &attacker, const Daemon &defender, const MoveData &move,
                    int randomPercent) {
    if (move.power <= 0)
        return 0;

    int attack = 0;
    int defense = 0;
    if (move.category == MoveCategory::special) {
        attack = attacker.getStat(3);
        defense = defender.getStat(4);
    } else {
        attack = attacker.getStat(1);
        defense = defender.getStat(2);
    }

    const int level = attacker.getLevel();
    const int power = move.power;
    int baseDamage = ((2 * level / 5 + 2) * power * attack / defense / 50) + 2;

    baseDamage = static_cast<int>(static_cast<float>(baseDamage) *
                                  effectivenessMultiplier(move.type, defender.getSpecies()));

    if (move.type == attacker.getSpecies().primaryType ||
        move.type == attacker.getSpecies().secondaryType) {
        baseDamage = baseDamage * 3 / 2;
    }

    baseDamage = baseDamage * randomPercent / 100;
    return std::max(1, baseDamage);
}

int calculateExpYield(const Daemon &defeated) {
    return defeated.getSpecies().baseExpYield * defeated.getLevel() / 7;
}

} // namespace BattleRules
