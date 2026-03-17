#include "SaveManager.h"
#include "../core/StringUtils.h"
#include "../core/TextParse.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using StringUtils::trimRightInPlace;

namespace {

enum class SaveSection { none, header, flags, badges, inventory, party, pc_boxes, npcs, daemondex };

SaveSection parseSectionHeader(const std::string &line) {
    if (line == "[header]")
        return SaveSection::header;
    if (line == "[flags]")
        return SaveSection::flags;
    if (line == "[badges]")
        return SaveSection::badges;
    if (line == "[inventory]")
        return SaveSection::inventory;
    if (line == "[party]")
        return SaveSection::party;
    if (line == "[pc_boxes]")
        return SaveSection::pc_boxes;
    if (line == "[npcs]")
        return SaveSection::npcs;
    if (line == "[daemondex]")
        return SaveSection::daemondex;
    return SaveSection::none;
}

} // namespace

SaveManager::SaveManager() : baseSavePath("saves/") {}

// --- BaseStats serialization ---

std::string SaveManager::serializeBaseStats(const BaseStats &stats) {
    return std::to_string(stats.hp) + "," + std::to_string(stats.attack) + "," +
           std::to_string(stats.defense) + "," + std::to_string(stats.specialAttack) + "," +
           std::to_string(stats.specialDefense) + "," + std::to_string(stats.speed);
}

BaseStats SaveManager::deserializeBaseStats(std::string_view s) {
    const auto values = TextParse::parseFixedIntList<6>(s, ',');
    if (!values.has_value())
        throw std::runtime_error("Invalid BaseStats encoding");

    return BaseStats{(*values)[0], (*values)[1], (*values)[2],
                     (*values)[3], (*values)[4], (*values)[5]};
}

// --- Daemon serialization ---
// Format:
// speciesId;level;exp;currentHP;nickname;status;iv_hp,iv_atk,...;ev_hp,ev_atk,...;moveId:curPP:maxPP,...

std::string SaveManager::serializeDaemon(const Daemon &daemon) {
    std::ostringstream oss;
    oss << daemon.getSpeciesId() << ";" << daemon.getLevel() << ";" << daemon.getExp() << ";"
        << daemon.getCurrentHP() << ";" << daemon.getNickname() << ";"
        << static_cast<int>(daemon.getStatus()) << ";" << serializeBaseStats(daemon.getIVs()) << ";"
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

Daemon SaveManager::deserializeDaemon(const std::string &line, const Pokedex &pokedex) {
    const auto parts = TextParse::splitView(line, ';');
    if (parts.size() != 9)
        throw std::runtime_error("Invalid daemon field count");

    const auto speciesId = TextParse::parseInt(parts[0]);
    const auto level = TextParse::parseInt(parts[1]);
    const auto exp = TextParse::parseInt(parts[2]);
    const auto currentHP = TextParse::parseInt(parts[3]);
    const auto statusRaw = TextParse::parseInt(parts[5]);
    if (!speciesId.has_value() || !level.has_value() || !exp.has_value() ||
        !currentHP.has_value() || !statusRaw.has_value()) {
        throw std::runtime_error("Invalid numeric daemon field");
    }

    std::string nickname(parts[4]);
    auto status = static_cast<StatusEffect>(*statusRaw);
    BaseStats ivs = deserializeBaseStats(parts[6]);
    BaseStats evs = deserializeBaseStats(parts[7]);

    // Parse moves: "moveId:curPP:maxPP,moveId:curPP:maxPP,..."
    std::array<MoveSlot, 4> moves{};
    for (auto &m : moves) {
        m.moveId = -1;
        m.currentPP = 0;
        m.maxPP = 0;
    }

    const auto moveParts = TextParse::splitView(parts[8], ',');
    int slot = 0;
    for (const auto moveToken : moveParts) {
        if (slot >= 4)
            break;
        const auto values = TextParse::parseFixedIntList<3>(moveToken, ':');
        if (!values.has_value())
            throw std::runtime_error("Invalid move slot encoding");
        moves[static_cast<std::size_t>(slot)].moveId = (*values)[0];
        moves[static_cast<std::size_t>(slot)].currentPP = (*values)[1];
        moves[static_cast<std::size_t>(slot)].maxPP = (*values)[2];
        ++slot;
    }

    const Species &species = pokedex.getSpecies(*speciesId);
    return Daemon(species, *level, *exp, *currentHP, nickname, status, ivs, evs, moves);
}

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

bool SaveManager::loadGame(const std::string &filepath, Player &player, World &world,
                           const Pokedex &pokedex) {
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

    SaveSection section = SaveSection::none;

    std::string line;
    while (std::getline(in, line)) {
        trimRightInPlace(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (SaveSection nextSection = parseSectionHeader(line); nextSection != SaveSection::none) {
            section = nextSection;
            continue;
        }

        switch (section) {
        case SaveSection::header: {
            const auto parts = TextParse::splitView(line, '|');
            if (parts.size() < 2)
                break;
            const std::string_view key = parts[0];
            if (key == "map")
                mapId = std::string(parts[1]);
            else if (key == "money") {
                if (const auto money = TextParse::parseInt(parts[1]); money.has_value())
                    player.setMoney(*money);
            } else if (key == "facing") {
                if (const auto dir = TextParse::parseInt(parts[1]); dir.has_value())
                    facing = static_cast<Direction>(*dir);
            } else if (key == "pos") {
                if (const auto coords = TextParse::parseFixedIntFields<2>(parts, 1);
                    coords.has_value()) {
                    pos.x = (*coords)[0];
                    pos.y = (*coords)[1];
                }
            }
            break;
        }
        case SaveSection::flags:
            player.setFlag(line);
            break;
        case SaveSection::badges:
            player.addBadge(line);
            break;
        case SaveSection::inventory: {
            const auto values = TextParse::parseFixedIntList<2>(line, '|');
            if (values.has_value()) {
                player.addItem((*values)[0], (*values)[1]);
            }
            break;
        }
        case SaveSection::party: {
            try {
                Daemon c = deserializeDaemon(line, pokedex);
                player.addDaemon(c);
            } catch (const std::exception &e) {
                std::cerr << "SaveManager: failed to load Daemon: " << e.what() << "\n";
            }
            break;
        }
        case SaveSection::pc_boxes: {
            if (line.starts_with("current_box|")) {
                if (const auto box = TextParse::parseInt(std::string_view(line).substr(12));
                    box.has_value()) {
                    player.setCurrentBox(*box);
                }
            } else if (line.starts_with("box|")) {
                const auto parts = TextParse::splitView(line, '|');
                if (parts.size() < 3)
                    break;
                std::string daemonData(parts[2]);
                try {
                    Daemon c = deserializeDaemon(daemonData, pokedex);
                    player.addDaemon(c); // Overflows to PC boxes
                } catch (const std::exception &e) {
                    std::cerr << "SaveManager: failed to load PC Daemon: " << e.what() << "\n";
                }
            }
            break;
        }
        case SaveSection::npcs: {
            const auto parts = TextParse::splitView(line, '|');
            if (parts.size() >= 3) {
                NPC *npc = world.findNPCById(std::string(parts[0]), std::string(parts[1]));
                if (npc)
                    npc->setDefeated(true);
            }
            break;
        }
        case SaveSection::daemondex: {
            const auto keyVal = TextParse::splitOnce(line, '|');
            if (keyVal.has_value()) {
                const auto id = TextParse::parseInt(keyVal->second);
                if (!id.has_value())
                    break;
                std::string_view key = keyVal->first;
                if (key == "seen")
                    player.markSeen(*id);
                else if (key == "caught")
                    player.markCaught(*id);
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
        trimRightInPlace(line);

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
            if (const auto keyVal = TextParse::splitOnce(line, '|'); keyVal.has_value()) {
                std::string_view key = keyVal->first;
                std::string_view val = keyVal->second;
                if (key == "name")
                    info.playerName = std::string(val);
                if (key == "map")
                    info.mapId = std::string(val);
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
