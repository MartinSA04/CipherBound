#pragma once
#include "Species.h"
#include <string>
#include <unordered_map>

enum class MoveCategory {
    physical,
    special,
    status,
};

inline const std::unordered_map<std::string, MoveCategory> categoryMap = {
    {"physical", MoveCategory::physical},
    {"special", MoveCategory::special},
    {"status", MoveCategory::status},
};

enum class StatusEffect {
    none,
    overheated, // Thermal overload — damage over time
    entangled,  // Quantum entanglement — can't switch out
    decayed,    // Radioactive decay — stats drop over time
    segfault,   // Memory error — random move failure
    deadlocked, // Concurrency deadlock — can't act
    paradox,    // Logical paradox — confused
};

inline const std::unordered_map<std::string, StatusEffect> statusMap = {
    {"none", StatusEffect::none},           {"overheated", StatusEffect::overheated},
    {"entangled", StatusEffect::entangled}, {"decayed", StatusEffect::decayed},
    {"segfault", StatusEffect::segfault},   {"deadlocked", StatusEffect::deadlocked},
    {"paradox", StatusEffect::paradox},
};

struct MoveData {
    int id;
    std::string name;
    std::string description;
    ElementType type;
    MoveCategory category;
    int power;
    int accuracy;
    int maxPP;
    StatusEffect statusEffect;
    int statusChance; // percentage 0-100
};
