#include "World.h"
#include "../data/Pokedex.h"
#include "../core/StringUtils.h"
#include <stdexcept>
#include <fstream>
#include <sstream>

using StringUtils::splitPipe;
using StringUtils::splitSemicolon;
using StringUtils::splitDoubleAt;
using StringUtils::parseDirection;

World::World(unsigned long seed)
    : seed(seed), rng(seed), currentMapId(""), player("Player", {5, 5})
{
}

namespace
{

    TileType charToTileType(char c)
    {
        switch (c)
        {
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

    bool isTileWalkable(TileType type)
    {
        switch (type)
        {
        case TileType::wall:
        case TileType::mountain:
        case TileType::water:
            return false;
        default:
            return true;
        }
    }

    NPCType parseNPCType(const std::string &s)
    {
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

std::string World::loadMap(const std::filesystem::path &path, const Pokedex &pokedex)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open map file: " + path.string());

    // Parse sections
    std::string mapId;
    int mapWidth = 0, mapHeight = 0;
    std::string bgImage;
    std::string bgImageOverlay;
    int spawnX = -1, spawnY = -1;

    std::vector<std::string> tileRows;
    std::vector<WarpPoint> warps;
    std::vector<WildEncounterSlot> encounters;

    // Raw NPC data from file
    struct RawNPC
    {
        std::string id, name, typeStr, facingStr;
        int x, y, sightRange;
        std::string dialogueStr, partyStr;
    };
    std::vector<RawNPC> rawNPCs;

    enum class Section
    {
        none,
        header,
        tiles,
        warps,
        encounters,
        npcs,
    };
    Section section = Section::none;

    std::string line;
    while (std::getline(file, line))
    {
        // Trim trailing whitespace
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();

        // Skip empty lines and comments
        if (line.empty())
            continue;
        if (section != Section::tiles && line[0] == '#')
            continue;

        // Section headers
        if (line == "[header]")
        {
            section = Section::header;
            continue;
        }
        if (line == "[tiles]")
        {
            section = Section::tiles;
            continue;
        }
        if (line == "[warps]")
        {
            section = Section::warps;
            continue;
        }
        if (line == "[encounters]")
        {
            section = Section::encounters;
            continue;
        }
        if (line == "[npcs]")
        {
            section = Section::npcs;
            continue;
        }

        switch (section)
        {
        case Section::header:
        {
            auto parts = splitPipe(line);
            if (parts.size() >= 2)
            {
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
                else if (key == "player_spawn" && parts.size() >= 3)
                {
                    spawnX = std::stoi(parts[1]);
                    spawnY = std::stoi(parts[2]);
                }
            }
            break;
        }
        case Section::tiles:
        {
            tileRows.push_back(line);
            break;
        }
        case Section::warps:
        {
            auto parts = splitPipe(line);
            if (parts.size() >= 5)
            {
                WarpPoint warp;
                warp.from = {std::stoi(parts[0]), std::stoi(parts[1])};
                warp.targetMapId = parts[2];
                warp.targetPosition = {std::stoi(parts[3]), std::stoi(parts[4])};
                warps.push_back(warp);
            }
            break;
        }
        case Section::encounters:
        {
            auto parts = splitPipe(line);
            if (parts.size() >= 4)
            {
                WildEncounterSlot slot;
                slot.speciesId = std::stoi(parts[0]);
                slot.minLevel = std::stoi(parts[1]);
                slot.maxLevel = std::stoi(parts[2]);
                slot.weight = std::stoi(parts[3]);
                encounters.push_back(slot);
            }
            break;
        }
        case Section::npcs:
        {
            // Format: id|name|type|x|y|facing|sightRange|dialogue|party
            auto parts = splitPipe(line);
            if (parts.size() >= 8)
            {
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

    // Build the Map object
    Map map(mapId, mapWidth, mapHeight);

    if (!bgImage.empty())
        map.setBackgroundImage(bgImage);
    if (!bgImageOverlay.empty())
        map.setBackgroundImageOverlay(bgImageOverlay);

    // Apply tile grid
    for (int y = 0; y < mapHeight && y < static_cast<int>(tileRows.size()); ++y)
    {
        const std::string &row = tileRows[y];
        for (int x = 0; x < mapWidth && x < static_cast<int>(row.size()); ++x)
        {
            TileType type = charToTileType(row[x]);
            Tile tile;
            tile.position = {x, y};
            tile.type = type;
            tile.isOccupied = false;
            tile.hasCollision = !isTileWalkable(type);
            tile.spriteId = 0;
            map.setTile({x, y}, tile);
        }
    }

    // Add warps
    for (auto &warp : warps)
        map.addWarp(warp);

    // Add encounters
    for (auto &slot : encounters)
        map.addEncounterSlot(slot);

    addMap(mapId, std::move(map));

    // Create NPCs from raw data
    for (const auto &raw : rawNPCs)
    {
        std::shared_ptr<NPC> npc = std::make_shared<NPC>(raw.id, raw.name, Position{raw.x, raw.y}, parseNPCType(raw.typeStr));
        npc->setFacing(parseDirection(raw.facingStr));
        npc->setSightRange(raw.sightRange);

        // Parse dialogue stages: "flag1?line1;line2@@flag2?line3;line4@@line5;line6"
        auto stages = splitDoubleAt(raw.dialogueStr);
        for (const auto &stageStr : stages)
        {
            if (stageStr.empty())
                continue;
            auto questionMark = stageStr.find('?');
            std::string flag;
            std::string linesStr;
            if (questionMark != std::string::npos)
            {
                flag = stageStr.substr(0, questionMark);
                linesStr = stageStr.substr(questionMark + 1);
            }
            else
            {
                linesStr = stageStr;
            }
            npc->addDialogueStage(flag, splitSemicolon(linesStr));
        }

        // Parse party: "speciesId:level,speciesId:level,..."
        if (!raw.partyStr.empty() && raw.partyStr != "-")
        {
            std::istringstream pss(raw.partyStr);
            std::string entry;
            while (std::getline(pss, entry, ','))
            {
                auto colon = entry.find(':');
                if (colon != std::string::npos)
                {
                    int speciesId = std::stoi(entry.substr(0, colon));
                    int level = std::stoi(entry.substr(colon + 1));
                    const Species &species = pokedex.getSpecies(speciesId);
                    npc->addDaemon(Daemon(species, level));
                }
            }
        }

        // Hide PC NPCs — their sprite is part of the map tileset
        if (npc->getType() == NPCType::pc)
            npc->setHidden(true);

        addNPC(mapId, std::move(npc));
    }

    // Set player spawn if specified
    if (spawnX >= 0 && spawnY >= 0)
        player = Player("Player", {spawnX, spawnY});

    return mapId;
}

void World::generate(const Pokedex &pokedex)
{
    // Load all Pallet Town maps from data files
    loadMap("assets/data/maps/pallet_town.map", pokedex);
    loadMap("assets/data/maps/house1_1f.map", pokedex);
    std::string startMap = loadMap("assets/data/maps/house1_2f.map", pokedex);
    loadMap("assets/data/maps/house2_1f.map", pokedex);
    loadMap("assets/data/maps/bart_iver_lab.map", pokedex);

    // route 1
    loadMap("assets/data/maps/route_1.map", pokedex);

    setCurrentMap(startMap);
}

// --- Map management ---

void World::addMap(const std::string &id, Map map)
{
    maps.emplace(id, std::move(map));
}

Map &World::getMap(const std::string &id)
{
    auto it = maps.find(id);
    if (it == maps.end())
        throw std::runtime_error("Map not found: " + id);
    return it->second;
}

const Map &World::getMap(const std::string &id) const
{
    auto it = maps.find(id);
    if (it == maps.end())
        throw std::runtime_error("Map not found: " + id);
    return it->second;
}

const std::string &World::getCurrentMapId() const { return currentMapId; }

std::vector<std::string> World::getMapIds() const
{
    std::vector<std::string> ids;
    ids.reserve(maps.size());
    for (const auto &[id, _] : maps)
        ids.push_back(id);
    return ids;
}

void World::setCurrentMap(const std::string &id)
{
    if (maps.find(id) == maps.end())
        throw std::runtime_error("Cannot set current map, not found: " + id);
    currentMapId = id;
}

// --- Player ---

Player &World::getPlayer() { return player; }
const Player &World::getPlayer() const { return player; }
void World::setPlayer(Player p) { player = std::move(p); }

// --- NPCs ---

void World::addNPC(const std::string &mapId, std::shared_ptr<NPC> npc)
{
    npcs[mapId].push_back(npc);
}

std::vector<std::shared_ptr<NPC>> &World::getNPCs(const std::string &mapId)
{
    return npcs[mapId];
}

const std::vector<std::shared_ptr<NPC>> &World::getNPCs(const std::string &mapId) const
{
    static const std::vector<std::shared_ptr<NPC>> empty;
    auto it = npcs.find(mapId);
    return (it != npcs.end()) ? it->second : empty;
}

std::shared_ptr<NPC> World::findNPCById(const std::string &npcId)
{
    auto &npcList = npcs[getCurrentMapId()];
    for (auto &npc : npcList)
    {
        if (npc->getId() == npcId)
            return npc;
    }
    return nullptr;
}

std::shared_ptr<NPC> World::findNPCById(const std::string &mapId, const std::string &npcId)
{
    auto &npcList = npcs[mapId];
    for (auto &npc : npcList)
    {
        if (npc->getId() == npcId)
            return npc;
    }
    return nullptr;
}

std::shared_ptr<NPC> World::findNPCAt(const std::string &mapId, const Position &pos)
{
    auto &npcList = npcs[mapId];
    for (auto &npc : npcList)
    {
        if (npc->getPosition() == pos)
            return npc;
    }
    return nullptr;
}

std::shared_ptr<NPC> World::NPCSeeingPlayer()
{
    for (auto &npc : npcs[getCurrentMapId()])
    {
        if (!npc->willFight())
            continue;

        if (npc->canSeePlayer(player.getPosition()))
        {
            return npc;
        }
    }
    return nullptr;
}

void World::setNPCDefeated(const std::string &npcId)
{
    getPlayer().setFlag("defeated_" + npcId);
    std::shared_ptr<NPC> currentTrainerNPC = findNPCById(npcId);
    if (currentTrainerNPC)
        currentTrainerNPC->setDefeated(true);
}

// --- Wild encounters ---

bool World::rollWildEncounter(const Position &position)
{
    const Map &map = getMap(currentMapId);
    if (!map.hasWildEncounters())
        return false;

    const Tile &tile = map.getTile(position);
    if (tile.type != TileType::tallGrass)
        return false;

    // ~15% chance per step in tall grass
    std::uniform_int_distribution<int> dist(0, 99);
    return dist(rng) < 15;
}

int World::getWildSpecies([[maybe_unused]] const Position &position)
{
    const Map &map = getMap(currentMapId);
    const auto &slots = map.getEncounterSlots();
    if (slots.empty())
        return 0;

    // Weighted random selection
    int totalWeight = 0;
    for (const auto &slot : slots)
        totalWeight += slot.weight;

    std::uniform_int_distribution<int> dist(0, totalWeight - 1);
    int roll = dist(rng);

    int accumulated = 0;
    for (const auto &slot : slots)
    {
        accumulated += slot.weight;
        if (roll < accumulated)
            return slot.speciesId;
    }
    return slots.back().speciesId;
}

int World::getWildLevel([[maybe_unused]] const Position &position)
{
    const Map &map = getMap(currentMapId);
    const auto &slots = map.getEncounterSlots();
    if (slots.empty())
        return 1;

    // Pick a random level within the first matching slot range
    // (simplified — ideally tied to getWildSpecies result)
    int minLvl = slots[0].minLevel;
    int maxLvl = slots[0].maxLevel;

    std::uniform_int_distribution<int> dist(minLvl, maxLvl);
    return dist(rng);
}

std::mt19937 &World::getRng() { return rng; }

void World::pushPlayerBack()
{
    Direction opposite;
    switch (player.getFacing())
    {
    case Direction::up:    opposite = Direction::down;  break;
    case Direction::down:  opposite = Direction::up;    break;
    case Direction::left:  opposite = Direction::right; break;
    case Direction::right: opposite = Direction::left;  break;
    }
    Map &map = getMap(currentMapId);
    map.setOccupied(player.getPosition(), false);
    player.move(opposite);
    map.setOccupied(player.getPosition(), true);
}
