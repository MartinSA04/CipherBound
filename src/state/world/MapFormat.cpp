#include "MapFormat.h"
#include "../../common/StringUtils.h"
#include "../../common/TextParse.h"

using StringUtils::parseDirection;
using StringUtils::splitDoubleAt;
using StringUtils::splitSemicolon;
using StringUtils::trimRightInPlace;

namespace MapFormat {
namespace {

enum class Section { none, header, tiles, warps, encounters, npcs };

NPCType parseNPCType(std::string_view value) {
    if (value == "trainer")
        return NPCType::trainer;
    if (value == "gymLeader")
        return NPCType::gymLeader;
    if (value == "shopkeeper")
        return NPCType::shopkeeper;
    if (value == "healer")
        return NPCType::healer;
    if (value == "questGiver")
        return NPCType::questGiver;
    if (value == "pc")
        return NPCType::pc;
    return NPCType::normal;
}

std::optional<WarpPoint> parseWarp(std::string_view line) {
    const auto parts = TextParse::splitView(line, '|');
    if (parts.size() != 5)
        return std::nullopt;

    const auto from = TextParse::parseFixedIntFields<2>(parts, 0);
    const auto target = TextParse::parseFixedIntFields<2>(parts, 3);
    if (!from.has_value() || !target.has_value())
        return std::nullopt;

    return WarpPoint{{(*from)[0], (*from)[1]}, std::string(parts[2]),
                     {(*target)[0], (*target)[1]}};
}

std::optional<WildEncounterSlot> parseEncounter(std::string_view line) {
    const auto values = TextParse::parseFixedIntList<4>(line, '|');
    if (!values.has_value())
        return std::nullopt;

    return WildEncounterSlot{(*values)[0], (*values)[1], (*values)[2], (*values)[3]};
}

std::vector<DialogueStage> parseDialogueStages(std::string_view encoded) {
    std::vector<DialogueStage> stages;
    for (const auto &stageString : splitDoubleAt(std::string(encoded))) {
        if (stageString.empty())
            continue;

        const auto questionMark = stageString.find('?');
        if (questionMark == std::string::npos) {
            stages.push_back(DialogueStage{"", splitSemicolon(stageString)});
            continue;
        }

        stages.push_back(DialogueStage{stageString.substr(0, questionMark),
                                       splitSemicolon(stageString.substr(questionMark + 1))});
    }
    return stages;
}

std::vector<NPCPartyMember> parseNPCParty(std::string_view encoded) {
    std::vector<NPCPartyMember> party;
    if (encoded.empty() || encoded == "-")
        return party;

    for (const auto entry : TextParse::splitView(encoded, ',')) {
        const auto values = TextParse::parseFixedIntList<2>(entry, ':');
        if (!values.has_value())
            continue;
        party.push_back(NPCPartyMember{(*values)[0], (*values)[1]});
    }
    return party;
}

std::optional<NPCDefinition> parseNPC(std::string_view line) {
    const auto parts = TextParse::splitView(line, '|');
    if (parts.size() != 10)
        return std::nullopt;

    const auto coords = TextParse::parseFixedIntFields<2>(parts, 3);
    const auto sightRange = TextParse::parseInt(parts[6]);
    if (!coords.has_value() || !sightRange.has_value() || parts[0].find('@') != std::string_view::npos)
        return std::nullopt;

    NPCDefinition npc;
    npc.id = std::string(parts[0]);
    npc.name = std::string(parts[1]);
    npc.type = parseNPCType(parts[2]);
    npc.position = {(*coords)[0], (*coords)[1]};
    npc.facing = parseDirection(std::string(parts[5]));
    npc.sightRange = *sightRange;
    npc.dialogueStages = parseDialogueStages(parts[7]);
    if (!parts[8].empty() && parts[8] != "-")
        npc.spriteType = std::string(parts[8]);
    npc.party = parseNPCParty(parts[9]);
    return npc;
}

} // namespace

ParseResult parse(std::istream &input) {
    ParseResult result;
    Section section = Section::none;

    std::string line;
    while (std::getline(input, line)) {
        trimRightInPlace(line);

        if (line.empty())
            continue;
        if (section != Section::tiles && line[0] == '#')
            continue;

        if (line == "[header]") {
            section = Section::header;
            continue;
        }
        if (line == "[tiles]") {
            section = Section::tiles;
            continue;
        }
        if (line == "[warps]") {
            section = Section::warps;
            continue;
        }
        if (line == "[encounters]") {
            section = Section::encounters;
            continue;
        }
        if (line == "[npcs]") {
            section = Section::npcs;
            continue;
        }

        switch (section) {
        case Section::header: {
            const auto parts = TextParse::splitView(line, '|');
            if (parts.size() < 2)
                break;

            const std::string_view key = parts[0];
            if (key == "id")
                result.definition.id = std::string(parts[1]);
            else if (key == "width") {
                if (const auto width = TextParse::parseInt(parts[1]); width.has_value())
                    result.definition.width = *width;
                else
                    result.warnings.push_back("Invalid width entry: " + line);
            } else if (key == "height") {
                if (const auto height = TextParse::parseInt(parts[1]); height.has_value())
                    result.definition.height = *height;
                else
                    result.warnings.push_back("Invalid height entry: " + line);
            } else if (key == "background")
                result.definition.backgroundImage = std::string(parts[1]);
            else if (key == "background_overlay")
                result.definition.backgroundImageOverlay = std::string(parts[1]);
            else if (key == "player_spawn") {
                if (const auto spawn = TextParse::parseFixedIntFields<2>(parts, 1);
                    spawn.has_value()) {
                    result.definition.playerSpawn = Position{(*spawn)[0], (*spawn)[1]};
                } else {
                    result.warnings.push_back("Invalid spawn entry: " + line);
                }
            }
            break;
        }
        case Section::tiles:
            result.definition.tileRows.push_back(line);
            break;
        case Section::warps: {
            const auto warp = parseWarp(line);
            if (!warp.has_value()) {
                result.warnings.push_back("Invalid warp entry: " + line);
                break;
            }
            result.definition.warps.push_back(*warp);
            break;
        }
        case Section::encounters: {
            const auto encounter = parseEncounter(line);
            if (!encounter.has_value()) {
                result.warnings.push_back("Invalid encounter entry: " + line);
                break;
            }
            result.definition.encounters.push_back(*encounter);
            break;
        }
        case Section::npcs: {
            const auto npc = parseNPC(line);
            if (!npc.has_value()) {
                result.warnings.push_back("Invalid NPC entry: " + line);
                break;
            }
            result.definition.npcs.push_back(*npc);
            break;
        }
        default:
            break;
        }
    }

    return result;
}

} // namespace MapFormat
