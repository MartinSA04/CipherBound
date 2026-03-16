#include "SaveManager.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

SaveManager::SaveManager() : baseSavePath("saves/") {}

// --- BaseStats serialization ---

std::string SaveManager::serializeBaseStats(const BaseStats &stats) {
    return std::to_string(stats.hp) + "," + std::to_string(stats.attack) + "," +
           std::to_string(stats.defense) + "," +
           std::to_string(stats.specialAttack) + "," +
           std::to_string(stats.specialDefense) + "," +
           std::to_string(stats.speed);
}

BaseStats SaveManager::deserializeBaseStats(const std::string &s) {
    BaseStats stats{0, 0, 0, 0, 0, 0};
    std::istringstream iss(s);
    std::string token;
    int idx = 0;
    while (std::getline(iss, token, ',') && idx < 6) {
        int val = std::stoi(token);
        switch (idx) {
        case 0:
            stats.hp = val;
            break;
        case 1:
            stats.attack = val;
            break;
        case 2:
            stats.defense = val;
            break;
        case 3:
            stats.specialAttack = val;
            break;
        case 4:
            stats.specialDefense = val;
            break;
        case 5:
            stats.speed = val;
            break;
        }
        ++idx;
    }
    return stats;
}

// --- Daemon serialization ---
// Format:
// speciesId;level;exp;currentHP;nickname;status;iv_hp,iv_atk,...;ev_hp,ev_atk,...;moveId:curPP:maxPP,...

std::string SaveManager::serializeDaemon(const Daemon &daemon) {
    std::ostringstream oss;
    oss << daemon.getSpeciesId() << ";" << daemon.getLevel() << ";"
        << daemon.getExp() << ";" << daemon.getCurrentHP() << ";"
        << daemon.getNickname() << ";" << static_cast<int>(daemon.getStatus())
        << ";" << serializeBaseStats(daemon.getIVs()) << ";"
        << serializeBaseStats(daemon.getEVs()) << ";";

    const auto &moves = daemon.getMoves();
    for (int i = 0; i < 4; ++i) {
        if (i > 0)
            oss << ",";
        oss << moves[static_cast<size_t>(i)].moveId << ":"
            << moves[static_cast<size_t>(i)].currentPP << ":"
            << moves[static_cast<size_t>(i)].maxPP;
    }
    return oss.str();
}

Daemon SaveManager::deserializeDaemon(const std::string &line,
                                      const Pokedex &pokedex) {
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> parts;
    while (std::getline(iss, token, ';'))
        parts.push_back(token);

    int speciesId = std::stoi(parts[0]);
    int level = std::stoi(parts[1]);
    int exp = std::stoi(parts[2]);
    int currentHP = std::stoi(parts[3]);
    std::string nickname = parts[4];
    auto status = static_cast<StatusEffect>(std::stoi(parts[5]));
    BaseStats ivs = deserializeBaseStats(parts[6]);
    BaseStats evs = deserializeBaseStats(parts[7]);

    // Parse moves: "moveId:curPP:maxPP,moveId:curPP:maxPP,..."
    std::array<MoveSlot, 4> moves{};
    for (auto &m : moves) {
        m.moveId = -1;
        m.currentPP = 0;
        m.maxPP = 0;
    }

    std::istringstream moveSS(parts[8]);
    std::string moveToken;
    int slot = 0;
    while (std::getline(moveSS, moveToken, ',') && slot < 4) {
        std::istringstream ms(moveToken);
        std::string mv;
        std::getline(ms, mv, ':');
        moves[static_cast<std::size_t>(slot)].moveId = std::stoi(mv);
        std::getline(ms, mv, ':');
        moves[static_cast<std::size_t>(slot)].currentPP = std::stoi(mv);
        std::getline(ms, mv, ':');
        moves[static_cast<std::size_t>(slot)].maxPP = std::stoi(mv);
        ++slot;
    }

    const Species &species = pokedex.getSpecies(speciesId);
    return Daemon(species, level, exp, currentHP, nickname, status, ivs, evs,
                  moves);
}

// --- Save game ---

bool SaveManager::saveGame(const std::string &filepath, const Player &player,
                           const World &world) {
    std::filesystem::create_directories(
        std::filesystem::path(filepath).parent_path());
    std::ofstream out(filepath);
    if (!out.is_open())
        return false;

    // [header]
    out << "[header]\n";
    out << "name|" << player.getName() << "\n";
    out << "map|" << world.getCurrentMapId() << "\n";
    out << "pos|" << player.getPosition().x << "|" << player.getPosition().y
        << "\n";
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
        out << serializeDaemon(daemon) << "\n";

    // [pc_boxes]
    out << "[pc_boxes]\n";
    out << "current_box|" << player.getCurrentBox() << "\n";
    for (int b = 0; b < Player::NUM_BOXES; ++b) {
        const auto &box = player.getBox(b);
        for (const auto &daemon : box)
            out << "box|" << b << "|" << serializeDaemon(daemon) << "\n";
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

bool SaveManager::loadGame(const std::string &filepath, Player &player,
                           World &world, const Pokedex &pokedex) {
    std::ifstream in(filepath);
    if (!in.is_open())
        return false;

    // Clear existing state
    player.clearParty();
    player.clearInventory();
    player.clearBadges();
    player.clearFlags();
    player.clearPCBoxes();
    player.clearDaemondex();
    player.setMoney(0);

    std::string mapId;
    Position pos{0, 0};
    Direction facing = Direction::down;

    enum class Section {
        none,
        header,
        flags,
        badges,
        inventory,
        party,
        pc_boxes,
        npcs,
        daemondex
    };
    Section section = Section::none;

    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();

        if (line.empty() || line[0] == '#')
            continue;

        if (line == "[header]") {
            section = Section::header;
            continue;
        }
        if (line == "[flags]") {
            section = Section::flags;
            continue;
        }
        if (line == "[badges]") {
            section = Section::badges;
            continue;
        }
        if (line == "[inventory]") {
            section = Section::inventory;
            continue;
        }
        if (line == "[party]") {
            section = Section::party;
            continue;
        }
        if (line == "[pc_boxes]") {
            section = Section::pc_boxes;
            continue;
        }
        if (line == "[npcs]") {
            section = Section::npcs;
            continue;
        }
        if (line == "[daemondex]") {
            section = Section::daemondex;
            continue;
        }

        switch (section) {
        case Section::header: {
            auto sep = line.find('|');
            if (sep == std::string::npos)
                break;
            std::string key = line.substr(0, sep);
            std::string val = line.substr(sep + 1);
            if (key == "map")
                mapId = val;
            else if (key == "money")
                player.setMoney(std::stoi(val));
            else if (key == "facing")
                facing = static_cast<Direction>(std::stoi(val));
            else if (key == "pos") {
                auto sep2 = val.find('|');
                pos.x = std::stoi(val.substr(0, sep2));
                pos.y = std::stoi(val.substr(sep2 + 1));
            }
            break;
        }
        case Section::flags:
            player.setFlag(line);
            break;
        case Section::badges:
            player.addBadge(line);
            break;
        case Section::inventory: {
            auto sep = line.find('|');
            if (sep != std::string::npos) {
                int itemId = std::stoi(line.substr(0, sep));
                int qty = std::stoi(line.substr(sep + 1));
                player.addItem(itemId, qty);
            }
            break;
        }
        case Section::party: {
            try {
                Daemon c = deserializeDaemon(line, pokedex);
                player.addDaemon(c);
            } catch (const std::exception &e) {
                std::cerr << "SaveManager: failed to load Daemon: " << e.what()
                          << "\n";
            }
            break;
        }
        case Section::pc_boxes: {
            if (line.starts_with("current_box|")) {
                player.setCurrentBox(std::stoi(line.substr(12)));
            } else if (line.starts_with("box|")) {
                auto first = line.find('|');
                auto second = line.find('|', first + 1);
                // int boxIdx = std::stoi(line.substr(first + 1, second - first
                // - 1));
                std::string daemonData = line.substr(second + 1);
                try {
                    Daemon c = deserializeDaemon(daemonData, pokedex);
                    player.addDaemon(c); // Overflows to PC boxes
                } catch (const std::exception &e) {
                    std::cerr
                        << "SaveManager: failed to load PC Daemon: " << e.what()
                        << "\n";
                }
            }
            break;
        }
        case Section::npcs: {
            auto sep1 = line.find('|');
            auto sep2 = line.find('|', sep1 + 1);
            if (sep1 != std::string::npos && sep2 != std::string::npos) {
                std::string npcMapId = line.substr(0, sep1);
                std::string npcId = line.substr(sep1 + 1, sep2 - sep1 - 1);
                std::shared_ptr<NPC> npc = world.findNPCById(npcMapId, npcId);
                if (npc)
                    npc->setDefeated(true);
            }
            break;
        }
        case Section::daemondex: {
            auto sep = line.find('|');
            if (sep != std::string::npos) {
                std::string key = line.substr(0, sep);
                int id = std::stoi(line.substr(sep + 1));
                if (key == "seen")
                    player.markSeen(id);
                else if (key == "caught")
                    player.markCaught(id);
            }
            break;
        }
        default:
            break;
        }
    }

    // Set map and position
    if (!mapId.empty()) {
        world.setCurrentMap(mapId);
        player.setPosition(pos);
        player.setFacing(facing);
    }

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

    std::string line;
    bool inHeader = false;
    bool inParty = false;
    bool inBadges = false;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();

        if (line == "[header]") {
            inHeader = true;
            inParty = false;
            inBadges = false;
            continue;
        }
        if (line == "[party]") {
            inParty = true;
            inHeader = false;
            inBadges = false;
            continue;
        }
        if (line == "[badges]") {
            inBadges = true;
            inHeader = false;
            inParty = false;
            continue;
        }
        if (!line.empty() && line[0] == '[') {
            inHeader = false;
            inParty = false;
            inBadges = false;
            continue;
        }

        if (inHeader) {
            auto sep = line.find('|');
            if (sep != std::string::npos) {
                std::string key = line.substr(0, sep);
                std::string val = line.substr(sep + 1);
                if (key == "name")
                    info.playerName = val;
                if (key == "map")
                    info.mapId = val;
            }
        }
        if (inParty && !line.empty())
            info.partySize++;
        if (inBadges && !line.empty())
            info.badgeCount++;
    }

    return info;
}

std::vector<SaveManager::SlotInfo> SaveManager::getSlotInfos() const {
    std::vector<SlotInfo> info;
    for (int i = 0; i < getSlotCount(); ++i)
        info.push_back(getSlotInfo(i));
    return info;
}
