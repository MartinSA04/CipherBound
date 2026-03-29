#include "../../common/FilePaths.h"
#include "../../game_data/Pokedex.h"
#include "../World.h"
#include "MapFormat.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

TileType charToTileType(char c) {
    switch (c) {
    case '.':
        return TileType::grass;
    case 'T':
        return TileType::tallGrass;
    case 'w':
        return TileType::water;
    case 'p':
        return TileType::path;
    case '#':
        return TileType::wall;
    case 'd':
        return TileType::door;
    case 'm':
        return TileType::mountain;
    case 's':
        return TileType::sand;
    case '^':
        return TileType::ledgeUp;
    case 'v':
        return TileType::ledgeDown;
    case '<':
        return TileType::ledgeLeft;
    case '>':
        return TileType::ledgeRight;
    default:
        return TileType::grass;
    }
}

bool isTileWalkable(TileType type) {
    switch (type) {
    case TileType::wall:
    case TileType::mountain:
    case TileType::water:
        return false;
    default:
        return true;
    }
}

std::vector<std::filesystem::path> discoverMapFiles(const std::filesystem::path &mapRoot) {
    std::vector<std::filesystem::path> mapFiles;
    if (!std::filesystem::exists(mapRoot))
        throw std::runtime_error("Map directory not found: " + mapRoot.string());

    for (const auto &entry : std::filesystem::recursive_directory_iterator(mapRoot)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".map")
            continue;
        mapFiles.push_back(entry.path());
    }

    std::sort(mapFiles.begin(), mapFiles.end());
    return mapFiles;
}

} // namespace

std::string World::loadMap(const std::filesystem::path &path, const Pokedex &pokedex) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open map file: " + path.string());

    auto parsed = MapFormat::parse(file);
    for (const auto &warning : parsed.warnings)
        std::cerr << "MapLoading(" << path.string() << "): " << warning << "\n";

    if (!parsed.valid())
        throw std::runtime_error("Invalid map file (missing id/width/height): " + path.string());

    const auto &definition = parsed.definition;
    Map map(definition.id, definition.width, definition.height);

    if (!definition.backgroundImage.empty())
        map.setBackgroundImage(definition.backgroundImage);
    if (!definition.backgroundImageOverlay.empty())
        map.setBackgroundImageOverlay(definition.backgroundImageOverlay);

    for (int y = 0; y < definition.height && y < static_cast<int>(definition.tileRows.size());
         ++y) {
        const std::string &row = definition.tileRows[static_cast<std::size_t>(y)];
        for (int x = 0; x < definition.width && x < static_cast<int>(row.size()); ++x) {
            TileType type = charToTileType(row[static_cast<std::size_t>(x)]);
            Tile tile;
            int tileX = x;
            int tileY = y;
            tile.position = {tileX, tileY};
            tile.type = type;
            tile.isOccupied = false;
            tile.hasCollision = !isTileWalkable(type);
            map.setTile({tileX, tileY}, tile);
        }
    }

    for (const auto &warp : definition.warps)
        map.addWarp(warp);

    for (const auto &slot : definition.encounters)
        map.addEncounterSlot(slot);

    addMap(definition.id, std::move(map));

    for (const auto &npcDefinition : definition.npcs) {
        auto npc =
            std::make_unique<NPC>(npcDefinition.id, npcDefinition.name, npcDefinition.position,
                                  npcDefinition.type, npcDefinition.spriteType);
        npc->setFacing(npcDefinition.facing);
        npc->setSightRange(npcDefinition.sightRange);

        for (const auto &stage : npcDefinition.dialogueStages)
            npc->addDialogueStage(stage.requiredFlag, stage.lines);

        for (const auto &entry : npcDefinition.party) {
            const Species &species = pokedex.getSpecies(entry.speciesId);
            npc->addDaemon(Daemon(species, entry.level));
        }

        if (npcDefinition.hidden || npc->getType() == NPCType::pc)
            npc->setHidden(true);

        npc->captureSpawnState();
        addNPC(definition.id, std::move(npc));
    }

    if (definition.playerSpawn.has_value()) {
        if (defaultRespawnMapId.empty()) {
            player = Player("Player", *definition.playerSpawn);
            setDefaultRespawnPoint(definition.id, *definition.playerSpawn, player.getFacing());
        } else {
            std::cerr << "MapLoading(" << path.string()
                      << "): ignoring additional player_spawn for map '" << definition.id << "'\n";
        }
    }

    return definition.id;
}

void World::generate(const Pokedex &pokedex) {
    maps.clear();
    npcs.clear();
    currentMapId.clear();
    defaultRespawnMapId.clear();
    defaultRespawnPosition = {0, 0};
    defaultRespawnFacing = Direction::down;
    player = Player("Player", {5, 5});

    const std::filesystem::path mapRoot = FilePaths::resolveExistingPath("assets/data/maps");
    const std::vector<std::filesystem::path> mapFiles = discoverMapFiles(mapRoot);
    if (mapFiles.empty())
        throw std::runtime_error("No map files found under: " + mapRoot.string());

    for (const auto &mapPath : mapFiles)
        loadMap(mapPath, pokedex);

    if (defaultRespawnMapId.empty())
        throw std::runtime_error("No map with player_spawn found under: " + mapRoot.string());

    setCurrentMap(defaultRespawnMapId);
    maps.at(currentMapId).setOccupied(player.getPosition(), true);
}
