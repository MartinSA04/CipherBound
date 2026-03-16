#include "../../core/StringUtils.h"
#include "../../data/Pokedex.h"
#include "../World.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

using StringUtils::parseDirection;
using StringUtils::splitDoubleAt;
using StringUtils::splitPipe;
using StringUtils::splitSemicolon;
using StringUtils::trimRightInPlace;

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

NPCType parseNPCType(const std::string &s) {
    if (s == "trainer")
        return NPCType::trainer;
    if (s == "gymLeader")
        return NPCType::gymLeader;
    if (s == "shopkeeper")
        return NPCType::shopkeeper;
    if (s == "healer")
        return NPCType::healer;
    if (s == "questGiver")
        return NPCType::questGiver;
    if (s == "pc")
        return NPCType::pc;
    return NPCType::normal;
}

} // namespace

std::string World::loadMap(const std::filesystem::path &path, const Pokedex &pokedex) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open map file: " + path.string());

    std::string mapId;
    int mapWidth = 0, mapHeight = 0;
    std::string bgImage;
    std::string bgImageOverlay;
    int spawnX = -1, spawnY = -1;

    std::vector<std::string> tileRows;
    std::vector<WarpPoint> warps;
    std::vector<WildEncounterSlot> encounters;

    struct RawNPC {
        std::string id, name, typeStr, facingStr;
        int x, y, sightRange;
        std::string dialogueStr, partyStr;
    };
    std::vector<RawNPC> rawNPCs;

    enum class Section {
        none,
        header,
        tiles,
        warps,
        encounters,
        npcs,
    };
    Section section = Section::none;

    std::string line;
    while (std::getline(file, line)) {
        trimRightInPlace(line);

        if (line.empty())
            continue;
        if (section != Section::tiles && line[0] == '#')
            continue;

        if (line == "[header]") {
            section = Section::header;
            continue;
        }
        if (line == "[tiles]") {
            section = Section::tiles;
            continue;
        }
        if (line == "[warps]") {
            section = Section::warps;
            continue;
        }
        if (line == "[encounters]") {
            section = Section::encounters;
            continue;
        }
        if (line == "[npcs]") {
            section = Section::npcs;
            continue;
        }

        switch (section) {
        case Section::header: {
            auto parts = splitPipe(line);
            if (parts.size() >= 2) {
                const std::string &key = parts[0];
                if (key == "id")
                    mapId = parts[1];
                else if (key == "width")
                    mapWidth = std::stoi(parts[1]);
                else if (key == "height")
                    mapHeight = std::stoi(parts[1]);
                else if (key == "background")
                    bgImage = parts[1];
                else if (key == "background_overlay")
                    bgImageOverlay = parts[1];
                else if (key == "player_spawn" && parts.size() >= 3) {
                    spawnX = std::stoi(parts[1]);
                    spawnY = std::stoi(parts[2]);
                }
            }
            break;
        }
        case Section::tiles:
            tileRows.push_back(line);
            break;
        case Section::warps: {
            auto parts = splitPipe(line);
            if (parts.size() >= 5) {
                WarpPoint warp;
                warp.from = {std::stoi(parts[0]), std::stoi(parts[1])};
                warp.targetMapId = parts[2];
                warp.targetPosition = {std::stoi(parts[3]), std::stoi(parts[4])};
                warps.push_back(warp);
            }
            break;
        }
        case Section::encounters: {
            auto parts = splitPipe(line);
            if (parts.size() >= 4) {
                WildEncounterSlot slot;
                slot.speciesId = std::stoi(parts[0]);
                slot.minLevel = std::stoi(parts[1]);
                slot.maxLevel = std::stoi(parts[2]);
                slot.weight = std::stoi(parts[3]);
                encounters.push_back(slot);
            }
            break;
        }
        case Section::npcs: {
            auto parts = splitPipe(line);
            if (parts.size() >= 8) {
                RawNPC raw;
                raw.id = parts[0];
                raw.name = parts[1];
                raw.typeStr = parts[2];
                raw.x = std::stoi(parts[3]);
                raw.y = std::stoi(parts[4]);
                raw.facingStr = parts[5];
                raw.sightRange = std::stoi(parts[6]);
                raw.dialogueStr = parts[7];
                raw.partyStr = (parts.size() > 8) ? parts[8] : "";
                rawNPCs.push_back(raw);
            }
            break;
        }
        default:
            break;
        }
    }

    if (mapId.empty() || mapWidth <= 0 || mapHeight <= 0)
        throw std::runtime_error("Invalid map file (missing id/width/height): " + path.string());

    Map map(mapId, mapWidth, mapHeight);

    if (!bgImage.empty())
        map.setBackgroundImage(bgImage);
    if (!bgImageOverlay.empty())
        map.setBackgroundImageOverlay(bgImageOverlay);

    for (int y = 0; y < mapHeight && y < static_cast<int>(tileRows.size()); ++y) {
        const std::string &row = tileRows[static_cast<std::size_t>(y)];
        for (int x = 0; x < mapWidth && x < static_cast<int>(row.size()); ++x) {
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

    for (auto &warp : warps)
        map.addWarp(warp);

    for (auto &slot : encounters)
        map.addEncounterSlot(slot);

    addMap(mapId, std::move(map));

    for (const auto &raw : rawNPCs) {
        std::shared_ptr<NPC> npc = std::make_shared<NPC>(raw.id, raw.name, Position{raw.x, raw.y},
                                                         parseNPCType(raw.typeStr));
        npc->setFacing(parseDirection(raw.facingStr));
        npc->setSightRange(raw.sightRange);

        auto stages = splitDoubleAt(raw.dialogueStr);
        for (const auto &stageStr : stages) {
            if (stageStr.empty())
                continue;
            auto questionMark = stageStr.find('?');
            std::string flag;
            std::string linesStr;
            if (questionMark != std::string::npos) {
                flag = stageStr.substr(0, questionMark);
                linesStr = stageStr.substr(questionMark + 1);
            } else {
                linesStr = stageStr;
            }
            npc->addDialogueStage(flag, splitSemicolon(linesStr));
        }

        if (!raw.partyStr.empty() && raw.partyStr != "-") {
            std::istringstream pss(raw.partyStr);
            std::string entry;
            while (std::getline(pss, entry, ',')) {
                auto colon = entry.find(':');
                if (colon != std::string::npos) {
                    int speciesId = std::stoi(entry.substr(0, colon));
                    int level = std::stoi(entry.substr(colon + 1));
                    const Species &species = pokedex.getSpecies(speciesId);
                    npc->addDaemon(Daemon(species, level));
                }
            }
        }

        if (npc->getType() == NPCType::pc)
            npc->setHidden(true);

        addNPC(mapId, std::move(npc));
    }

    if (spawnX >= 0 && spawnY >= 0)
        player = Player("Player", {spawnX, spawnY});

    return mapId;
}

void World::generate(const Pokedex &pokedex) {
    loadMap("assets/data/maps/pallet_town.map", pokedex);
    loadMap("assets/data/maps/house1_1f.map", pokedex);
    std::string startMap = loadMap("assets/data/maps/house1_2f.map", pokedex);
    loadMap("assets/data/maps/house2_1f.map", pokedex);
    loadMap("assets/data/maps/bart_iver_lab.map", pokedex);

    loadMap("assets/data/maps/route_1.map", pokedex);

    setCurrentMap(startMap);
}
