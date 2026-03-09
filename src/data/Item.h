#pragma once
#include <string>

enum class ItemCategory
{
    healing,
    capture,
    battle,
    keyItem,
};

struct ItemData
{
    int id;
    std::string name;
    std::string description;
    ItemCategory category;
    int value;       // buy price
    int effectValue; // e.g. heal amount, catch rate modifier
};
