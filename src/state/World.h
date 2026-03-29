/**
 * @file
 * @brief World-level ownership of maps, NPCs, player state, and encounter logic.
 * @ingroup world_state
 */

#pragma once

#include "Map.h"
#include "NPC.h"
#include "player/Player.h"
#include <filesystem>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

class Pokedex; // forward declaration

/**
 * @brief Owns loaded maps, NPC instances, the player, and world-scoped runtime state.
 * @ingroup world_state
 */
class World {
  public:
    /// Owned NPC collection for a single map id.
    using NPCList = std::vector<std::unique_ptr<NPC>>;

    /// Creates a world with a deterministic RNG seed.
    World(int seed);

    /// Generates baseline world state from loaded game data.
    void generate(const Pokedex &pokedex);

    /// Loads a `.map` file and returns the map id parsed from its header.
    std::string loadMap(const std::filesystem::path &path, const Pokedex &pokedex);

    /// Adds a fully constructed map under the given id.
    void addMap(const std::string &id, Map map);
    /// Returns mutable access to a loaded map.
    Map &getMap(const std::string &id);
    /// Returns immutable access to a loaded map.
    const Map &getMap(const std::string &id) const;
    /// Returns ids for all loaded maps.
    std::vector<std::string> getMapIds() const;
    /// Returns the id of the currently active map.
    const std::string &getCurrentMapId() const;
    /// Sets the currently active map.
    void setCurrentMap(const std::string &id);

    /// Returns mutable access to the player entity.
    Player &getPlayer();
    /// Returns immutable access to the player entity.
    const Player &getPlayer() const;
    /// Replaces the player object stored by the world.
    void setPlayer(Player player);
    /// Fully heals the player's current party.
    void healPlayerParty();
    /// Sets the default respawn point used after blackout.
    void setDefaultRespawnPoint(const std::string &mapId, Position position,
                                Direction facing = Direction::down);
    /// Updates the respawn point to the current healing center.
    void markHealingCenterUsed(Position position, Direction facing);
    /// Respawns the player at the stored blackout recovery point.
    void respawnPlayerAfterBlackout();

    /// Adds an NPC to a specific map. Ownership is transferred to the world.
    void addNPC(const std::string &mapId, std::unique_ptr<NPC> npc);
    /// Returns mutable NPC storage for one map.
    NPCList &getNPCs(const std::string &mapId);
    /// Returns immutable NPC storage for one map.
    const NPCList &getNPCs(const std::string &mapId) const;
    /// Finds an NPC occupying the given position on the given map.
    NPC *findNPCAt(const std::string &mapId, const Position &pos, bool includeHidden = true);
    /// Finds an NPC by id across all loaded maps.
    NPC *findNPCById(const std::string &npcId);
    /// Finds an NPC by id within one map.
    NPC *findNPCById(const std::string &mapId, const std::string &npcId);
    /// Returns the first trainer NPC currently seeing the player, if any.
    NPC *NPCSeeingPlayer();
    /// Marks a trainer NPC as defeated.
    void setNPCDefeated(const std::string &npcId);

    /// Pushes the player one tile opposite the current facing direction.
    void pushPlayerBack();

    /// Rolls whether a wild encounter occurs on the given tile.
    bool rollWildEncounter(const Position &position);
    /// Chooses a wild species for the given tile.
    int getWildSpecies(const Position &position);
    /// Chooses a wild level for the given tile.
    int getWildLevel(const Position &position);

    /// Exposes the world RNG for systems that need coordinated randomness.
    std::mt19937 &getRng();

  private:
    std::mt19937 rng;
    std::string currentMapId;
    std::map<std::string, Map> maps;
    std::map<std::string, NPCList> npcs;
    Player player;
    std::string defaultRespawnMapId;
    Position defaultRespawnPosition{0, 0};
    Direction defaultRespawnFacing{Direction::down};
};
