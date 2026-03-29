#include "../World.h"

void World::addNPC(const std::string &mapId, std::unique_ptr<NPC> npc) {
    npcs[mapId].push_back(std::move(npc));
}

World::NPCList &World::getNPCs(const std::string &mapId) { return npcs[mapId]; }

const World::NPCList &World::getNPCs(const std::string &mapId) const {
    static const NPCList empty;
    auto it = npcs.find(mapId);
    return (it != npcs.end()) ? it->second : empty;
}

NPC *World::findNPCById(const std::string &npcId) {
    auto it = npcs.find(getCurrentMapId());
    if (it == npcs.end())
        return nullptr;

    for (auto &npc : it->second) {
        if (npc->getId() == npcId)
            return npc.get();
    }
    return nullptr;
}

NPC *World::findNPCById(const std::string &mapId, const std::string &npcId) {
    auto it = npcs.find(mapId);
    if (it == npcs.end())
        return nullptr;

    for (auto &npc : it->second) {
        if (npc->getId() == npcId)
            return npc.get();
    }
    return nullptr;
}

NPC *World::findNPCAt(const std::string &mapId, const Position &pos, bool includeHidden) {
    auto it = npcs.find(mapId);
    if (it == npcs.end())
        return nullptr;

    for (auto &npc : it->second) {
        if (!includeHidden && npc->isHidden())
            continue;
        if (npc->getPosition() == pos)
            return npc.get();
    }
    return nullptr;
}

NPC *World::NPCSeeingPlayer() {
    auto it = npcs.find(getCurrentMapId());
    if (it == npcs.end())
        return nullptr;

    for (auto &npc : it->second) {
        if (!npc->willFight())
            continue;

        if (npc->canSeePlayer(player.getPosition()))
            return npc.get();
    }
    return nullptr;
}

void World::setNPCDefeated(const std::string &npcId) {
    getPlayer().setFlag("defeated_" + npcId);
    NPC *currentTrainerNPC = findNPCById(npcId);
    if (currentTrainerNPC)
        currentTrainerNPC->setDefeated(true);
}
