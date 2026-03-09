#pragma once
#include <vector>
#include <string>
#include <functional>

enum class Direction
{
    up,
    down,
    left,
    right,
};

struct Position
{
    int x, y;

    bool operator==(const Position &other) const { return x == other.x && y == other.y; }
    bool operator!=(const Position &other) const { return !(*this == other); }
    void moveDirection(const Direction &dir)
    {
        switch (dir)
        {
        case Direction::up:
            y--;
            break;
        case Direction::down:
            y++;
            break;
        case Direction::left:
            x--;
            break;
        case Direction::right:
            x++;
            break;
        }
    }
};

enum class TileType
{
    grass,
    tallGrass, // wild encounters possible
    water,
    sand,
    path,
    mountain,
    wall,
    door,
    ledgeUp,
    ledgeDown,
    ledgeLeft,
    ledgeRight,
};

struct WildEncounterSlot
{
    int speciesId;
    int minLevel;
    int maxLevel;
    int weight; // relative probability
};

struct WarpPoint
{
    Position from;
    std::string targetMapId;
    Position targetPosition;
};

struct Tile
{
    Position position;
    TileType type;
    bool isOccupied;
    bool hasCollision;
    int spriteId;
};

class Map
{
public:
    Map();
    Map(const std::string &id, int width, int height);

    const std::string &getId() const;
    int getWidth() const;
    int getHeight() const;

    Tile &getTile(const Position &position);
    const Tile &getTile(const Position &position) const;
    void setTile(const Position &position, Tile tile);

    void setOccupied(const Position &position, bool occupied);

    bool isInBounds(const Position &position) const;
    bool isWalkable(const Position &position) const;

    // Wild encounters
    void addEncounterSlot(WildEncounterSlot slot);
    const std::vector<WildEncounterSlot> &getEncounterSlots() const;
    bool hasWildEncounters() const;

    // Warp points (doors, cave entrances, etc.)
    void addWarp(WarpPoint warp);
    const WarpPoint *getWarp(const Position &position) const;

    // Directional walkability check (needed for ledges)
    bool isWalkable(const Position &position, Direction fromDirection) const;

    // Background image for tile rendering
    void setBackgroundImage(const std::string &path);
    const std::string &getBackgroundImage() const;
    bool hasBackgroundImage() const;

    // Background image overlay for tile rendering
    void setBackgroundImageOverlay(const std::string &path);
    const std::string &getBackgroundImageOverlay() const;
    bool hasBackgroundImageOverlay() const;

private:
    std::string id;
    int width;
    int height;
    std::string backgroundImagePath;
    std::string backgroundImageOverlayPath;
    std::vector<std::vector<Tile>> grid;
    std::vector<WildEncounterSlot> encounterSlots;
    std::vector<WarpPoint> warps;
};