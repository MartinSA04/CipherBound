#pragma once

#include "Map.h"
#include "NPC.h"
#include "Player.h"
#include <filesystem>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

class Pokedex; // forward declaration

class World {
  public:
    World(int seed);

    void generate(const Pokedex &pokedex);

    // Load a map from a .map description file
    // Returns the map id read from the file
    std::string loadMap(const std::filesystem::path &path,
                        const Pokedex &pokedex);

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

    // NPCs on current map
    void addNPC(const std::string &mapId, std::shared_ptr<NPC> npc);
    std::vector<std::shared_ptr<NPC>> &getNPCs(const std::string &mapId);
    const std::vector<std::shared_ptr<NPC>> &
    getNPCs(const std::string &mapId) const;
    std::shared_ptr<NPC> findNPCAt(const std::string &mapId,
                                   const Position &pos);
    std::shared_ptr<NPC> findNPCById(const std::string &npcId);
    std::shared_ptr<NPC> findNPCById(const std::string &mapId,
                                     const std::string &npcId);
    std::shared_ptr<NPC> NPCSeeingPlayer();
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
    std::map<std::string, std::vector<std::shared_ptr<NPC>>> npcs;
    Player player;
};
