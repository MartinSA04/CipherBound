#include "SaveFormat.h"
#include "../common/StringUtils.h"
#include "../common/TextParse.h"
#include "../game_data/Pokedex.h"
#include <sstream>

using StringUtils::trimRightInPlace;

namespace SaveFormat {
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

std::string serializeBaseStats(const BaseStats &stats) {
    return std::to_string(stats.hp) + "," + std::to_string(stats.attack) + "," +
           std::to_string(stats.defense) + "," + std::to_string(stats.specialAttack) + "," +
           std::to_string(stats.specialDefense) + "," + std::to_string(stats.speed);
}

std::optional<BaseStats> parseBaseStats(std::string_view encoded) {
    const auto values = TextParse::parseFixedIntList<6>(encoded, ',');
    if (!values.has_value())
        return std::nullopt;

    return BaseStats{(*values)[0], (*values)[1], (*values)[2],
                     (*values)[3], (*values)[4], (*values)[5]};
}

std::optional<SavedDaemonData> parseDaemon(std::string_view line) {
    const auto parts = TextParse::splitView(line, ';');
    if (parts.size() != 9)
        return std::nullopt;

    const auto speciesId = TextParse::parseInt(parts[0]);
    const auto level = TextParse::parseInt(parts[1]);
    const auto exp = TextParse::parseInt(parts[2]);
    const auto currentHP = TextParse::parseInt(parts[3]);
    const auto statusRaw = TextParse::parseInt(parts[5]);
    const auto ivs = parseBaseStats(parts[6]);
    const auto evs = parseBaseStats(parts[7]);
    if (!speciesId.has_value() || !level.has_value() || !exp.has_value() ||
        !currentHP.has_value() || !statusRaw.has_value() || !ivs.has_value() ||
        !evs.has_value()) {
        return std::nullopt;
    }

    SavedDaemonData saved;
    saved.speciesId = *speciesId;
    saved.level = *level;
    saved.exp = *exp;
    saved.currentHP = *currentHP;
    saved.nickname = std::string(parts[4]);
    saved.status = static_cast<StatusEffect>(*statusRaw);
    saved.ivs = *ivs;
    saved.evs = *evs;

    const auto moveParts = TextParse::splitView(parts[8], ',');
    int slot = 0;
    for (const auto moveToken : moveParts) {
        if (slot >= 4)
            break;
        const auto values = TextParse::parseFixedIntList<3>(moveToken, ':');
        if (!values.has_value())
            return std::nullopt;
        saved.moves[static_cast<std::size_t>(slot)] = {(*values)[0], (*values)[1], (*values)[2]};
        ++slot;
    }

    return saved;
}

void ensureBoxStorage(std::vector<std::vector<SavedDaemonData>> &boxes, int boxIndex) {
    if (boxIndex < 0)
        return;
    const std::size_t requiredSize = static_cast<std::size_t>(boxIndex + 1);
    if (boxes.size() < requiredSize)
        boxes.resize(requiredSize);
}

} // namespace

std::string serializeDaemon(const Daemon &daemon) {
    std::ostringstream encoded;
    encoded << daemon.getSpeciesId() << ";" << daemon.getLevel() << ";" << daemon.getExp() << ";"
            << daemon.getCurrentHP() << ";" << daemon.getNickname() << ";"
            << static_cast<int>(daemon.getStatus()) << ";" << serializeBaseStats(daemon.getIVs())
            << ";" << serializeBaseStats(daemon.getEVs()) << ";";

    const auto &moves = daemon.getMoves();
    for (int i = 0; i < 4; ++i) {
        if (i > 0)
            encoded << ",";
        const auto &move = moves[static_cast<std::size_t>(i)];
        encoded << move.moveId << ":" << move.currentPP << ":" << move.maxPP;
    }
    return encoded.str();
}

SaveParseResult parse(std::istream &input) {
    SaveParseResult result;
    SaveSection section = SaveSection::none;

    std::string line;
    while (std::getline(input, line)) {
        trimRightInPlace(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (const SaveSection nextSection = parseSectionHeader(line);
            nextSection != SaveSection::none) {
            section = nextSection;
            continue;
        }

        switch (section) {
        case SaveSection::header: {
            const auto parts = TextParse::splitView(line, '|');
            if (parts.size() < 2)
                break;

            const std::string_view key = parts[0];
            if (key == "name")
                result.data.playerName = std::string(parts[1]);
            else if (key == "map")
                result.data.mapId = std::string(parts[1]);
            else if (key == "money") {
                if (const auto money = TextParse::parseInt(parts[1]); money.has_value())
                    result.data.money = *money;
                else
                    result.warnings.push_back("Invalid money value: " + line);
            } else if (key == "facing") {
                if (const auto dir = TextParse::parseInt(parts[1]); dir.has_value())
                    result.data.facing = static_cast<Direction>(*dir);
                else
                    result.warnings.push_back("Invalid facing value: " + line);
            } else if (key == "pos") {
                if (const auto coords = TextParse::parseFixedIntFields<2>(parts, 1);
                    coords.has_value()) {
                    result.data.position = {(*coords)[0], (*coords)[1]};
                } else {
                    result.warnings.push_back("Invalid position value: " + line);
                }
            }
            break;
        }
        case SaveSection::flags:
            result.data.flags.push_back(line);
            break;
        case SaveSection::badges:
            result.data.badges.push_back(line);
            break;
        case SaveSection::inventory: {
            const auto values = TextParse::parseFixedIntList<2>(line, '|');
            if (!values.has_value()) {
                result.warnings.push_back("Invalid inventory entry: " + line);
                break;
            }
            result.data.inventory.push_back(SavedInventoryEntry{(*values)[0], (*values)[1]});
            break;
        }
        case SaveSection::party: {
            const auto daemon = parseDaemon(line);
            if (!daemon.has_value()) {
                result.warnings.push_back("Invalid party daemon: " + line);
                break;
            }
            result.data.party.push_back(*daemon);
            break;
        }
        case SaveSection::pc_boxes: {
            if (line.starts_with("current_box|")) {
                const auto box = TextParse::parseInt(std::string_view(line).substr(12));
                if (box.has_value())
                    result.data.currentBox = *box;
                else
                    result.warnings.push_back("Invalid current box entry: " + line);
                break;
            }

            if (!line.starts_with("box|")) {
                result.warnings.push_back("Invalid PC box entry: " + line);
                break;
            }

            const auto parts = TextParse::splitView(line, '|');
            if (parts.size() < 3) {
                result.warnings.push_back("Invalid PC box entry: " + line);
                break;
            }

            const auto boxIndex = TextParse::parseInt(parts[1]);
            const auto daemon = parseDaemon(parts[2]);
            if (!boxIndex.has_value() || !daemon.has_value() || *boxIndex < 0) {
                result.warnings.push_back("Invalid PC box daemon: " + line);
                break;
            }

            ensureBoxStorage(result.data.pcBoxes, *boxIndex);
            result.data.pcBoxes[static_cast<std::size_t>(*boxIndex)].push_back(*daemon);
            break;
        }
        case SaveSection::npcs: {
            const auto parts = TextParse::splitView(line, '|');
            if (parts.size() < 3) {
                result.warnings.push_back("Invalid NPC state entry: " + line);
                break;
            }
            result.data.npcStates.push_back(
                SavedNPCState{std::string(parts[0]), std::string(parts[1]), parts[2] == "defeated"});
            break;
        }
        case SaveSection::daemondex: {
            const auto keyVal = TextParse::splitOnce(line, '|');
            if (!keyVal.has_value()) {
                result.warnings.push_back("Invalid daemondex entry: " + line);
                break;
            }

            const auto speciesId = TextParse::parseInt(keyVal->second);
            if (!speciesId.has_value()) {
                result.warnings.push_back("Invalid daemondex entry: " + line);
                break;
            }

            if (keyVal->first == "seen")
                result.data.seen.insert(*speciesId);
            else if (keyVal->first == "caught")
                result.data.caught.insert(*speciesId);
            else
                result.warnings.push_back("Unknown daemondex key: " + line);
            break;
        }
        default:
            break;
        }
    }

    return result;
}

SaveSlotSummary summarize(const SaveFileData &data) {
    SaveSlotSummary summary;
    summary.playerName = data.playerName;
    summary.partySize = static_cast<int>(data.party.size());
    summary.badgeCount = static_cast<int>(data.badges.size());
    summary.mapId = data.mapId;
    return summary;
}

Daemon hydrateDaemon(const SavedDaemonData &saved, const Pokedex &pokedex) {
    const Species &species = pokedex.getSpecies(saved.speciesId);
    return Daemon(species, saved.level, saved.exp, saved.currentHP, saved.nickname, saved.status,
                  saved.ivs, saved.evs, saved.moves);
}

} // namespace SaveFormat
