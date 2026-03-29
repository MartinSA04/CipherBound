#include "World.h"
#include <stdexcept>

World::World(int seed)
    : rng(static_cast<std::mt19937::result_type>(seed)), currentMapId(""),
      player("Player", {5, 5}) {}

// --- Map management ---

void World::addMap(const std::string &id, Map map) {
    const auto [_, inserted] = maps.emplace(id, std::move(map));
    if (!inserted)
        throw std::runtime_error("Duplicate map id: " + id);
}

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

    if (const auto it = npcs.find(currentMapId); it != npcs.end()) {
        for (auto &npc : it->second)
            npc->resetToSpawnState();
    }
}

// --- Player ---

Player &World::getPlayer() { return player; }
const Player &World::getPlayer() const { return player; }
void World::setPlayer(Player p) { player = std::move(p); }
void World::healPlayerParty() { player.fullHealParty(); }

void World::setDefaultRespawnPoint(const std::string &mapId, Position position, Direction facing) {
    defaultRespawnMapId = mapId;
    defaultRespawnPosition = position;
    defaultRespawnFacing = facing;
}

void World::markHealingCenterUsed(Position position, Direction facing) {
    player.setRespawnPoint(currentMapId, position, facing);
}

void World::respawnPlayerAfterBlackout() {
    healPlayerParty();

    std::string targetMapId;
    Position targetPosition{0, 0};
    Direction targetFacing = Direction::down;

    if (player.hasRespawnPoint() && maps.contains(player.getRespawnMapId())) {
        targetMapId = player.getRespawnMapId();
        targetPosition = player.getRespawnPosition();
        targetFacing = player.getRespawnFacing();
    } else if (!defaultRespawnMapId.empty() && maps.contains(defaultRespawnMapId)) {
        targetMapId = defaultRespawnMapId;
        targetPosition = defaultRespawnPosition;
        targetFacing = defaultRespawnFacing;
    }

    if (targetMapId.empty())
        return;

    if (!currentMapId.empty() && maps.contains(currentMapId))
        maps[currentMapId].setOccupied(player.getPosition(), false);

    setCurrentMap(targetMapId);
    player.setPosition(targetPosition);
    player.setFacing(targetFacing);
    maps[currentMapId].setOccupied(player.getPosition(), true);
}

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
