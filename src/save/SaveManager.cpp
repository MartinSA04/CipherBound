#include "SaveManager.h"
#include "SaveFormat.h"
#include <filesystem>
#include <fstream>
#include <iostream>

SaveManager::SaveManager() : baseSavePath("saves/") {}

// --- Save game ---

bool SaveManager::saveGame(const std::string &filepath, const Player &player, const World &world) {
    std::filesystem::create_directories(std::filesystem::path(filepath).parent_path());
    std::ofstream out(filepath);
    if (!out.is_open())
        return false;

    // [header]
    out << "[header]\n";
    out << "name|" << player.getName() << "\n";
    out << "map|" << world.getCurrentMapId() << "\n";
    out << "pos|" << player.getPosition().x << "|" << player.getPosition().y << "\n";
    out << "facing|" << static_cast<int>(player.getFacing()) << "\n";
    out << "money|" << player.getMoney() << "\n";

    // [flags]
    out << "[flags]\n";
    for (const auto &flag : player.getFlags())
        out << flag << "\n";

    // [badges]
    out << "[badges]\n";
    for (const auto &badge : player.getBadges())
        out << badge << "\n";

    // [inventory]
    out << "[inventory]\n";
    for (const auto &entry : player.getInventory())
        out << entry.itemId << "|" << entry.quantity << "\n";

    // [party]
    out << "[party]\n";
    for (const auto &daemon : player.getParty())
        out << SaveFormat::serializeDaemon(daemon) << "\n";

    // [pc_boxes]
    out << "[pc_boxes]\n";
    out << "current_box|" << player.getCurrentBox() << "\n";
    for (int b = 0; b < Player::NUM_BOXES; ++b) {
        const auto &box = player.getBox(b);
        for (const auto &daemon : box)
            out << "box|" << b << "|" << SaveFormat::serializeDaemon(daemon) << "\n";
    }

    // [npcs] — save defeated state
    out << "[npcs]\n";
    for (const auto &mapId : world.getMapIds()) {
        const auto &npcs = world.getNPCs(mapId);
        for (const auto &npc : npcs) {
            if (npc->isDefeated())
                out << mapId << "|" << npc->getId() << "|defeated\n";
        }
    }

    // [daemondex]
    out << "[daemondex]\n";
    for (int id : player.getSeenSet())
        out << "seen|" << id << "\n";
    for (int id : player.getCaughtSet())
        out << "caught|" << id << "\n";

    return true;
}

// --- Load game ---

bool SaveManager::loadGame(const std::string &filepath, Player &player, World &world,
                           const Pokedex &pokedex) {
    std::ifstream in(filepath);
    if (!in.is_open())
        return false;

    const SaveFormat::SaveParseResult parsed = SaveFormat::parse(in);
    for (const auto &warning : parsed.warnings)
        std::cerr << "SaveManager: " << warning << "\n";

    const std::string restoredName =
        parsed.data.playerName.empty() ? player.getName() : parsed.data.playerName;
    Player restored(restoredName, parsed.data.position);
    restored.setFacing(parsed.data.facing);
    restored.setMoney(parsed.data.money);

    for (const auto &flag : parsed.data.flags)
        restored.setFlag(flag);
    for (const auto &badge : parsed.data.badges)
        restored.addBadge(badge);
    for (const auto &entry : parsed.data.inventory)
        restored.addItem(entry.itemId, entry.quantity);
    for (const auto &speciesId : parsed.data.seen)
        restored.markSeen(speciesId);
    for (const auto &speciesId : parsed.data.caught)
        restored.markCaught(speciesId);

    for (const auto &saved : parsed.data.party) {
        try {
            restored.restorePartyDaemon(SaveFormat::hydrateDaemon(saved, pokedex));
        } catch (const std::exception &e) {
            std::cerr << "SaveManager: failed to restore party daemon: " << e.what() << "\n";
        }
    }

    for (std::size_t boxIndex = 0; boxIndex < parsed.data.pcBoxes.size(); ++boxIndex) {
        for (const auto &saved : parsed.data.pcBoxes[boxIndex]) {
            try {
                restored.restoreBoxDaemon(static_cast<int>(boxIndex),
                                          SaveFormat::hydrateDaemon(saved, pokedex));
            } catch (const std::exception &e) {
                std::cerr << "SaveManager: failed to restore boxed daemon: " << e.what() << "\n";
            }
        }
    }
    restored.setCurrentBox(parsed.data.currentBox);

    for (const auto &mapId : world.getMapIds()) {
        for (const auto &npc : world.getNPCs(mapId))
            npc->setDefeated(false);
    }
    for (const auto &state : parsed.data.npcStates) {
        if (!state.defeated)
            continue;
        if (NPC *npc = world.findNPCById(state.mapId, state.npcId); npc != nullptr)
            npc->setDefeated(true);
    }

    if (!world.getCurrentMapId().empty())
        world.getMap(world.getCurrentMapId()).setOccupied(player.getPosition(), false);

    if (!parsed.data.mapId.empty())
        world.setCurrentMap(parsed.data.mapId);

    world.setPlayer(std::move(restored));
    if (!world.getCurrentMapId().empty())
        world.getMap(world.getCurrentMapId()).setOccupied(world.getPlayer().getPosition(), true);

    return true;
}

bool SaveManager::saveFileExists(const std::string &filepath) const {
    return std::filesystem::exists(filepath);
}

bool SaveManager::deleteSave(const std::string &filepath) {
    return std::filesystem::remove(filepath);
}

std::string SaveManager::getSavePath(int slot) const {
    return baseSavePath + "slot_" + std::to_string(slot) + ".sav";
}

int SaveManager::getSlotCount() const { return MAX_SAVE_SLOTS; }

SaveManager::SlotInfo SaveManager::getSlotInfo(int slot) const {
    SlotInfo info;
    std::string path = getSavePath(slot);
    if (!std::filesystem::exists(path))
        return info;

    std::ifstream in(path);
    if (!in.is_open())
        return info;

    info.exists = true;

    const SaveFormat::SaveSlotSummary summary = SaveFormat::summarize(SaveFormat::parse(in).data);
    info.playerName = summary.playerName;
    info.partySize = summary.partySize;
    info.badgeCount = summary.badgeCount;
    info.mapId = summary.mapId;

    return info;
}

std::vector<SaveManager::SlotInfo> SaveManager::getSlotInfos() const {
    std::vector<SlotInfo> info;
    for (int i = 0; i < getSlotCount(); ++i)
        info.push_back(getSlotInfo(i));
    return info;
}
