#include "World.h"
#include <stdexcept>

World::World(int seed)
    : rng(static_cast<std::mt19937::result_type>(seed)), currentMapId(""),
      player("Player", {5, 5}) {}

// --- Map management ---

void World::addMap(const std::string &id, Map map) { maps.emplace(id, std::move(map)); }

Map &World::getMap(const std::string &id) {
    auto it = maps.find(id);
    if (it == maps.end())
        throw std::runtime_error("Map not found: " + id);
    return it->second;
}

const Map &World::getMap(const std::string &id) const {
    auto it = maps.find(id);
    if (it == maps.end())
        throw std::runtime_error("Map not found: " + id);
    return it->second;
}

const std::string &World::getCurrentMapId() const { return currentMapId; }

std::vector<std::string> World::getMapIds() const {
    std::vector<std::string> ids;
    ids.reserve(maps.size());
    for (const auto &[id, _] : maps)
        ids.push_back(id);
    return ids;
}

void World::setCurrentMap(const std::string &id) {
    if (maps.find(id) == maps.end())
        throw std::runtime_error("Cannot set current map, not found: " + id);
    currentMapId = id;
}

// --- Player ---

Player &World::getPlayer() { return player; }
const Player &World::getPlayer() const { return player; }
void World::setPlayer(Player p) { player = std::move(p); }

std::mt19937 &World::getRng() { return rng; }

void World::pushPlayerBack() {
    Direction opposite;
    switch (player.getFacing()) {
    case Direction::up:
        opposite = Direction::down;
        break;
    case Direction::down:
        opposite = Direction::up;
        break;
    case Direction::left:
        opposite = Direction::right;
        break;
    case Direction::right:
        opposite = Direction::left;
        break;
    }
    Map &map = getMap(currentMapId);
    map.setOccupied(player.getPosition(), false);
    player.move(opposite);
    map.setOccupied(player.getPosition(), true);
}
