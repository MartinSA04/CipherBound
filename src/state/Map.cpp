#include "Map.h"
#include <stdexcept>
std::out_of_range out_of_range_error(const Position &position)
{
    return std::out_of_range("Tile position out of bounds: (" +
                             std::to_string(position.x) + ", " +
                             std::to_string(position.y) + ")");
}

Map::Map() : id(""), width(0), height(0) {}

Map::Map(const std::string &id, int width, int height)
    : id(id), width(width), height(height)
{
    grid.resize(height);
    for (int y = 0; y < height; ++y)
    {
        grid[y].resize(width);
        for (int x = 0; x < width; ++x)
        {
            Tile tile;
            tile.position = {x, y};
            tile.type = TileType::grass;
            tile.isOccupied = false;
            tile.hasCollision = false;
            grid[y][x] = tile;
        }
    }
}

const std::string &Map::getId() const { return id; }
int Map::getWidth() const { return width; }
int Map::getHeight() const { return height; }

Tile &Map::getTile(const Position &position)
{
    if (!isInBounds(position))
        throw out_of_range_error(position);
    return grid[position.y][position.x];
}

const Tile &Map::getTile(const Position &position) const
{
    if (!isInBounds(position))
        throw out_of_range_error(position);
    return grid[position.y][position.x];
}

void Map::setTile(const Position &position, Tile tile)
{
    if (!isInBounds(position))
        throw out_of_range_error(position);
    tile.position = position; // ensure position consistency
    grid[position.y][position.x] = tile;
}

void Map::setOccupied(const Position &position, bool occupied)
{
    if (isInBounds(position))
        grid[position.y][position.x].isOccupied = occupied;
}

bool Map::isInBounds(const Position &position) const
{
    return position.x >= 0 && position.x < width &&
           position.y >= 0 && position.y < height;
}

bool Map::isWalkable(const Position &position) const
{
    if (!isInBounds(position))
        return false;

    const Tile &tile = grid[position.y][position.x];

    if (tile.isOccupied || tile.hasCollision)
        return false;

    switch (tile.type)
    {
    case TileType::wall:
    case TileType::mountain:
    case TileType::water:
    case TileType::ledgeUp:
    case TileType::ledgeDown:
    case TileType::ledgeLeft:
    case TileType::ledgeRight:
        return false;
    default:
        return true;
    }
}

bool Map::isWalkable(const Position &position, Direction fromDirection) const
{
    if (!isInBounds(position))
        return false;

    const Tile &tile = grid[position.y][position.x];

    if (tile.isOccupied || tile.hasCollision)
        return false;

    switch (tile.type)
    {
    case TileType::wall:
    case TileType::mountain:
    case TileType::water:
        return false;
    case TileType::ledgeDown:
        return fromDirection == Direction::down;
    case TileType::ledgeLeft:
        return fromDirection == Direction::left;
    case TileType::ledgeRight:
        return fromDirection == Direction::right;
    case TileType::ledgeUp:
        return fromDirection == Direction::up;
    default:
        return true;
    }
}

void Map::addEncounterSlot(WildEncounterSlot slot)
{
    encounterSlots.push_back(slot);
}

const std::vector<WildEncounterSlot> &Map::getEncounterSlots() const
{
    return encounterSlots;
}

bool Map::hasWildEncounters() const
{
    return !encounterSlots.empty();
}

void Map::addWarp(WarpPoint warp)
{
    warps.push_back(warp);
}

const WarpPoint *Map::getWarp(const Position &position) const
{
    for (const auto &warp : warps)
    {
        if (warp.from == position)
            return &warp;
    }
    return nullptr;
}

void Map::setBackgroundImage(const std::string &path)
{
    backgroundImagePath = path;
}

const std::string &Map::getBackgroundImage() const
{
    return backgroundImagePath;
}

bool Map::hasBackgroundImage() const
{
    return !backgroundImagePath.empty();
}

void Map::setBackgroundImageOverlay(const std::string &path)
{
    backgroundImageOverlayPath = path;
}

const std::string &Map::getBackgroundImageOverlay() const
{
    return backgroundImageOverlayPath;
}

bool Map::hasBackgroundImageOverlay() const
{
    return !backgroundImageOverlayPath.empty();
}