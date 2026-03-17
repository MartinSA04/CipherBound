#pragma once

#include "../Map.h"
#include "../NPC.h"
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace MapFormat {

struct NPCPartyMember {
    int speciesId{0};
    int level{0};
};

struct NPCDefinition {
    std::string id;
    std::string spriteType;
    std::string name;
    NPCType type{NPCType::normal};
    Position position{0, 0};
    Direction facing{Direction::down};
    int sightRange{0};
    std::vector<DialogueStage> dialogueStages;
    std::vector<NPCPartyMember> party;
};

struct MapDefinition {
    std::string id;
    int width{0};
    int height{0};
    std::string backgroundImage;
    std::string backgroundImageOverlay;
    std::optional<Position> playerSpawn;
    std::vector<std::string> tileRows;
    std::vector<WarpPoint> warps;
    std::vector<WildEncounterSlot> encounters;
    std::vector<NPCDefinition> npcs;
};

struct ParseResult {
    MapDefinition definition;
    std::vector<std::string> warnings;

    bool valid() const {
        return !definition.id.empty() && definition.width > 0 && definition.height > 0;
    }
};

ParseResult parse(std::istream &input);

} // namespace MapFormat
