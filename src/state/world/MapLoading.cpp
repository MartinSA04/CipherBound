#include "../../core/StringUtils.h"
#include "../../core/TextParse.h"
#include "../../data/Pokedex.h"
#include "../World.h"
#include <fstream>
#include <stdexcept>

using StringUtils::parseDirection;
using StringUtils::splitDoubleAt;
using StringUtils::splitSemicolon;
using StringUtils::trimRightInPlace;

namespace {

struct RawNPC {
    std::string id;
    std::string name;
    std::string typeStr;
    std::string facingStr;
    int x{0};
    int y{0};
    int sightRange{0};
    std::string dialogueStr;
    std::string partyStr;
};

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

std::optional<WarpPoint> parseWarpLine(std::string_view line) {
    const auto parts = TextParse::splitView(line, '|');
    if (parts.size() != 5)
        return std::nullopt;

    const auto from = TextParse::parseFixedIntFields<2>(parts, 0);
    const auto target = TextParse::parseFixedIntFields<2>(parts, 3);
    if (!from.has_value() || !target.has_value())
        return std::nullopt;

    WarpPoint warp;
    warp.from = {(*from)[0], (*from)[1]};
    warp.targetMapId = std::string(parts[2]);
    warp.targetPosition = {(*target)[0], (*target)[1]};
    return warp;
}

std::optional<WildEncounterSlot> parseEncounterLine(std::string_view line) {
    const auto values = TextParse::parseFixedIntList<4>(line, '|');
    if (!values.has_value())
        return std::nullopt;

    WildEncounterSlot slot;
    slot.speciesId = (*values)[0];
    slot.minLevel = (*values)[1];
    slot.maxLevel = (*values)[2];
    slot.weight = (*values)[3];
    return slot;
}

std::optional<RawNPC> parseNPCLine(std::string_view line) {
    const auto parts = TextParse::splitView(line, '|');
    if (parts.size() < 8)
        return std::nullopt;

    const auto coords = TextParse::parseFixedIntFields<2>(parts, 3);
    const auto sightRange = TextParse::parseInt(parts[6]);
    if (!coords.has_value() || !sightRange.has_value())
        return std::nullopt;

    RawNPC raw;
    raw.id = std::string(parts[0]);
    raw.name = std::string(parts[1]);
    raw.typeStr = std::string(parts[2]);
    raw.x = (*coords)[0];
    raw.y = (*coords)[1];
    raw.facingStr = std::string(parts[5]);
    raw.sightRange = *sightRange;
    raw.dialogueStr = std::string(parts[7]);
    raw.partyStr = (parts.size() > 8) ? std::string(parts[8]) : "";
    return raw;
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
            const auto parts = TextParse::splitView(line, '|');
            if (parts.size() >= 2) {
                const std::string_view key = parts[0];
                if (key == "id")
                    mapId = std::string(parts[1]);
                else if (key == "width") {
                    if (const auto width = TextParse::parseInt(parts[1]); width.has_value())
                        mapWidth = *width;
                } else if (key == "height") {
                    if (const auto height = TextParse::parseInt(parts[1]); height.has_value())
                        mapHeight = *height;
                } else if (key == "background")
                    bgImage = std::string(parts[1]);
                else if (key == "background_overlay")
                    bgImageOverlay = std::string(parts[1]);
                else if (key == "player_spawn") {
                    if (const auto spawn = TextParse::parseFixedIntFields<2>(parts, 1);
                        spawn.has_value()) {
                        spawnX = (*spawn)[0];
                        spawnY = (*spawn)[1];
                    }
                }
            }
            break;
        }
        case Section::tiles:
            tileRows.push_back(line);
            break;
        case Section::warps:
            if (const auto warp = parseWarpLine(line); warp.has_value())
                warps.push_back(*warp);
            break;
        case Section::encounters:
            if (const auto encounter = parseEncounterLine(line); encounter.has_value())
                encounters.push_back(*encounter);
            break;
        case Section::npcs:
            if (const auto npc = parseNPCLine(line); npc.has_value())
                rawNPCs.push_back(*npc);
            break;
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
        auto npc = std::make_unique<NPC>(raw.id, raw.name, Position{raw.x, raw.y},
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
            for (const auto entry : TextParse::splitView(raw.partyStr, ',')) {
                const auto values = TextParse::parseFixedIntList<2>(entry, ':');
                if (!values.has_value())
                    continue;
                const Species &species = pokedex.getSpecies((*values)[0]);
                npc->addDaemon(Daemon(species, (*values)[1]));
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
