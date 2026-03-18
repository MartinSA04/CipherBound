#include "../../game_data/Pokedex.h"
#include "../World.h"
#include "MapFormat.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

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

    for (int y = 0; y < definition.height && y < static_cast<int>(definition.tileRows.size()); ++y) {
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
        auto npc = std::make_unique<NPC>(npcDefinition.id, npcDefinition.name, npcDefinition.position,
                                         npcDefinition.type, npcDefinition.spriteType);
        npc->setFacing(npcDefinition.facing);
        npc->setSightRange(npcDefinition.sightRange);

        for (const auto &stage : npcDefinition.dialogueStages)
            npc->addDialogueStage(stage.requiredFlag, stage.lines);

        for (const auto &entry : npcDefinition.party) {
            const Species &species = pokedex.getSpecies(entry.speciesId);
            npc->addDaemon(Daemon(species, entry.level));
        }

        if (npc->getType() == NPCType::pc)
            npc->setHidden(true);

        addNPC(definition.id, std::move(npc));
    }

    if (definition.playerSpawn.has_value())
        player = Player("Player", *definition.playerSpawn);

    return definition.id;
}

void World::generate(const Pokedex &pokedex) {
    loadMap("assets/data/maps/pallet_town.map", pokedex);
    loadMap("assets/data/maps/viridian_town.map", pokedex);
    loadMap("assets/data/maps/house1_1f.map", pokedex);
    std::string startMap = loadMap("assets/data/maps/house1_2f.map", pokedex);
    loadMap("assets/data/maps/house2_1f.map", pokedex);
    loadMap("assets/data/maps/bart_iver_lab.map", pokedex);

    loadMap("assets/data/maps/route_1.map", pokedex);

    setDefaultRespawnPoint(startMap, player.getPosition(), player.getFacing());
    setCurrentMap(startMap);
}
