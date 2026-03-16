#include "../World.h"

bool World::rollWildEncounter(const Position &position) {
    const Map &map = getMap(currentMapId);
    if (!map.hasWildEncounters())
        return false;

    const Tile &tile = map.getTile(position);
    if (tile.type != TileType::tallGrass)
        return false;

    std::uniform_int_distribution<int> dist(0, 99);
    return dist(rng) < 15;
}

int World::getWildSpecies([[maybe_unused]] const Position &position) {
    const Map &map = getMap(currentMapId);
    const auto &slots = map.getEncounterSlots();
    if (slots.empty())
        return 0;

    int totalWeight = 0;
    for (const auto &slot : slots)
        totalWeight += slot.weight;

    std::uniform_int_distribution<int> dist(0, totalWeight - 1);
    int roll = dist(rng);

    int accumulated = 0;
    for (const auto &slot : slots) {
        accumulated += slot.weight;
        if (roll < accumulated)
            return slot.speciesId;
    }
    return slots.back().speciesId;
}

int World::getWildLevel([[maybe_unused]] const Position &position) {
    const Map &map = getMap(currentMapId);
    const auto &slots = map.getEncounterSlots();
    if (slots.empty())
        return 1;

    int minLvl = slots[0].minLevel;
    int maxLvl = slots[0].maxLevel;

    std::uniform_int_distribution<int> dist(minLvl, maxLvl);
    return dist(rng);
}
