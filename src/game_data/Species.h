#pragma once
#include <string>
#include <unordered_map>
#include <vector>

enum class GrowthRate {
    erratic,
    fast,
    mediumFast,
    mediumSlow,
    slow,
    fluctuating,
};

enum class ElementType {
    classical,       // Newtonian mechanics
    electromagnetic, // EM force, Maxwell's equations
    quantum,         // Quantum mechanics, wave-particle duality
    nuclear,         // Strong nuclear force
    radioactive,     // Weak force, decay
    thermal,         // Thermodynamics, entropy
    gravitational,   // Gravity, general relativity
    algebraic,       // Abstract algebra, group theory
    calculus,        // Analysis, integrals, limits
    probabilistic,   // Statistics, probability
    algorithmic,     // Algorithms, complexity theory
    logic,           // Boolean logic, formal proofs
    digital,         // Binary, hardware, circuits
    recursive,       // Recursion, self-reference
    chaotic,         // Chaos theory, fractals
};

inline const std::unordered_map<std::string, ElementType> typeMap = {
    {"classical", ElementType::classical},
    {"electromagnetic", ElementType::electromagnetic},
    {"quantum", ElementType::quantum},
    {"nuclear", ElementType::nuclear},
    {"radioactive", ElementType::radioactive},
    {"thermal", ElementType::thermal},
    {"gravitational", ElementType::gravitational},
    {"algebraic", ElementType::algebraic},
    {"calculus", ElementType::calculus},
    {"probabilistic", ElementType::probabilistic},
    {"algorithmic", ElementType::algorithmic},
    {"logic", ElementType::logic},
    {"digital", ElementType::digital},
    {"recursive", ElementType::recursive},
    {"chaotic", ElementType::chaotic},
};

inline const std::unordered_map<std::string, GrowthRate> growthRateMap = {
    {"erratic", GrowthRate::erratic},
    {"fast", GrowthRate::fast},
    {"mediumFast", GrowthRate::mediumFast},
    {"mediumSlow", GrowthRate::mediumSlow},
    {"slow", GrowthRate::slow},
    {"fluctuating", GrowthRate::fluctuating},
};

inline std::string elementTypeName(ElementType t) {
    for (const auto &[name, val] : typeMap) {
        if (val == t)
            return name;
    }
    return "???";
}

struct BaseStats {
    int hp;
    int attack;
    int defense;
    int specialAttack;
    int specialDefense;
    int speed;
};

struct EvolutionInfo {
    int targetSpeciesId;
    int levelRequired;
};

struct LearnableMove {
    int moveId;
    int levelLearned;
};

struct Species {
    int id;
    std::string name;
    ElementType primaryType;
    ElementType secondaryType;
    GrowthRate growthRate{GrowthRate::mediumFast};
    BaseStats baseStats;
    BaseStats effortYield{0, 0, 0, 0, 0, 0};
    int catchRate;
    int baseExpYield;
    std::vector<LearnableMove> learnset;
    std::vector<EvolutionInfo> evolutions;
};
