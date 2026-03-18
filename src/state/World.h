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

class World {
  public:
    using NPCList = std::vector<std::unique_ptr<NPC>>;

    World(int seed);

    void generate(const Pokedex &pokedex);

    // Load a map from a .map description file
    // Returns the map id read from the file
    std::string loadMap(const std::filesystem::path &path, const Pokedex &pokedex);

    // Map management
    void addMap(const std::string &id, Map map);
    Map &getMap(const std::string &id);
    const Map &getMap(const std::string &id) const;
    std::vector<std::string> getMapIds() const;
    const std::string &getCurrentMapId() const;
    void setCurrentMap(const std::string &id);

    // Player
    Player &getPlayer();
    const Player &getPlayer() const;
    void setPlayer(Player player);
    void healPlayerParty();
    void setDefaultRespawnPoint(const std::string &mapId, Position position,
                                Direction facing = Direction::down);
    void markHealingCenterUsed(Position position, Direction facing);
    void respawnPlayerAfterBlackout();

    // NPCs are owned by the world and exposed through non-owning pointers.
    void addNPC(const std::string &mapId, std::unique_ptr<NPC> npc);
    NPCList &getNPCs(const std::string &mapId);
    const NPCList &getNPCs(const std::string &mapId) const;
    NPC *findNPCAt(const std::string &mapId, const Position &pos);
    NPC *findNPCById(const std::string &npcId);
    NPC *findNPCById(const std::string &mapId, const std::string &npcId);
    NPC *NPCSeeingPlayer();
    void setNPCDefeated(const std::string &npcId);

    // Push the player one tile backwards (opposite of current facing)
    void pushPlayerBack();

    // Wild encounter check
    bool rollWildEncounter(const Position &position);
    int getWildSpecies(const Position &position);
    int getWildLevel(const Position &position);

    // Random engine
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
