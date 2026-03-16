#include "../World.h"

void World::addNPC(const std::string &mapId, std::shared_ptr<NPC> npc) {
    npcs[mapId].push_back(npc);
}

std::vector<std::shared_ptr<NPC>> &World::getNPCs(const std::string &mapId) { return npcs[mapId]; }

const std::vector<std::shared_ptr<NPC>> &World::getNPCs(const std::string &mapId) const {
    static const std::vector<std::shared_ptr<NPC>> empty;
    auto it = npcs.find(mapId);
    return (it != npcs.end()) ? it->second : empty;
}

std::shared_ptr<NPC> World::findNPCById(const std::string &npcId) {
    auto &npcList = npcs[getCurrentMapId()];
    for (auto &npc : npcList) {
        if (npc->getId() == npcId)
            return npc;
    }
    return nullptr;
}

std::shared_ptr<NPC> World::findNPCById(const std::string &mapId, const std::string &npcId) {
    auto &npcList = npcs[mapId];
    for (auto &npc : npcList) {
        if (npc->getId() == npcId)
            return npc;
    }
    return nullptr;
}

std::shared_ptr<NPC> World::findNPCAt(const std::string &mapId, const Position &pos) {
    auto &npcList = npcs[mapId];
    for (auto &npc : npcList) {
        if (npc->getPosition() == pos)
            return npc;
    }
    return nullptr;
}

std::shared_ptr<NPC> World::NPCSeeingPlayer() {
    for (auto &npc : npcs[getCurrentMapId()]) {
        if (!npc->willFight())
            continue;

        if (npc->canSeePlayer(player.getPosition()))
            return npc;
    }
    return nullptr;
}

void World::setNPCDefeated(const std::string &npcId) {
    getPlayer().setFlag("defeated_" + npcId);
    std::shared_ptr<NPC> currentTrainerNPC = findNPCById(npcId);
    if (currentTrainerNPC)
        currentTrainerNPC->setDefeated(true);
}
