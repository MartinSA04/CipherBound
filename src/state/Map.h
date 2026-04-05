/**
 * @file
 * @brief Tile map representation, encounter slots, and warp metadata.
 * @ingroup world_state
 */

#pragma once
#include "Movement.h"
#include <string>
#include <vector>

/// Semantic tile categories used for collision and encounter rules.
enum class TileType {
    grass,      ///< Regular grass tile.
    tallGrass,  ///< Tall grass where wild encounters may occur.
    water,      ///< Water tile.
    sand,       ///< Sand tile.
    path,       ///< Walkable path tile.
    mountain,   ///< Mountain terrain.
    wall,       ///< Blocking wall tile.
    door,       ///< Door or entrance tile.
    ledgeUp,    ///< One-way ledge entered from below.
    ledgeDown,  ///< One-way ledge entered from above.
    ledgeLeft,  ///< One-way ledge entered from the right.
    ledgeRight, ///< One-way ledge entered from the left.
};

/// Weighted wild-encounter entry for one map.
struct WildEncounterSlot {
    int speciesId; ///< Species id that may be rolled.
    int minLevel;  ///< Minimum generated level.
    int maxLevel;  ///< Maximum generated level.
    int weight;    ///< Relative selection weight among encounter slots.
};

/// Map transition trigger located on one tile.
struct WarpPoint {
    Position from;           ///< Source tile that triggers the warp.
    std::string targetMapId; ///< Destination map id.
    Position targetPosition; ///< Spawn tile on the destination map.
};

/// Runtime tile data used by collision, occupancy, and rendering.
struct Tile {
    Position position; ///< Tile coordinate inside the map grid.
    TileType type;     ///< Semantic tile type.
    bool isOccupied;   ///< Whether an entity currently occupies the tile.
    bool hasCollision; ///< Whether the tile blocks movement.
};

/**
 * @brief Runtime tile map with background art, warps, and encounter tables.
 * @ingroup world_state
 */
class Map {
  public:
    /// Constructs an empty map.
    Map();
    /// Constructs a map with a fixed size and identifier.
    Map(const std::string &id, int width, int height);

    /// Returns the map id.
    const std::string &getId() const;
    /// Returns the map width in tiles.
    int getWidth() const;
    /// Returns the map height in tiles.
    int getHeight() const;

    /// Returns mutable tile data at the given position.
    Tile &getTile(const Position &position);
    /// Returns immutable tile data at the given position.
    const Tile &getTile(const Position &position) const;
    /// Replaces the tile stored at the given position.
    void setTile(const Position &position, Tile tile);

    /// Marks a tile as occupied or free.
    void setOccupied(const Position &position, bool occupied);

    /// Returns whether the position lies inside the map bounds.
    bool isInBounds(const Position &position) const;
    /// Returns whether the tile is walkable without directional ledge rules.
    bool isWalkable(const Position &position) const;
    /// Returns whether the tile is walkable when entered from the given direction.
    bool isWalkable(const Position &position, Direction fromDirection) const;

    /// Adds a weighted wild-encounter entry.
    void addEncounterSlot(WildEncounterSlot slot);
    /// Returns the encounter table for this map.
    const std::vector<WildEncounterSlot> &getEncounterSlots() const;
    /// Returns whether this map can trigger wild encounters.
    bool hasWildEncounters() const;

    /// Adds a warp trigger.
    void addWarp(WarpPoint warp);
    /// Returns the warp at the given position, or `nullptr` if none exists.
    const WarpPoint *getWarp(const Position &position) const;

    /// Sets the background tileset image path.
    void setBackgroundImage(const std::string &path);
    /// Returns the background tileset image path.
    const std::string &getBackgroundImage() const;
    /// Returns whether a background tileset image has been configured.
    bool hasBackgroundImage() const;

    /// Sets the overlay image path rendered above the base background.
    void setBackgroundImageOverlay(const std::string &path);
    /// Returns the overlay image path.
    const std::string &getBackgroundImageOverlay() const;
    /// Returns whether an overlay image has been configured.
    bool hasBackgroundImageOverlay() const;

    /// Sets the looping background music file path for this map.
    void setMusicPath(const std::string &path);
    /// Returns the configured background music file path.
    const std::string &getMusicPath() const;
    /// Returns whether a map-specific music file has been configured.
    bool hasMusicPath() const;

  private:
    std::string id;
    int width;
    int height;
    std::string backgroundImagePath;
    std::string backgroundImageOverlayPath;
    std::string musicPath;
    std::vector<std::vector<Tile>> grid;
    std::vector<WildEncounterSlot> encounterSlots;
    std::vector<WarpPoint> warps;
};
