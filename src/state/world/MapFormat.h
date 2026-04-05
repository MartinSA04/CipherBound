/**
 * @file
 * @brief Parsed schema for `.map` files.
 * @ingroup world_state
 * @ingroup data_formats
 */

#pragma once

#include "../Map.h"
#include "../NPC.h"
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief Parser for section-based map definition files.
 * @ingroup data_formats
 */
namespace MapFormat {

/// One party member entry embedded in an NPC definition.
struct NPCPartyMember {
    int speciesId{0}; ///< Species id for the party member.
    int level{0};     ///< Starting level for the party member.
};

/// Parsed NPC definition from the `[npcs]` section of a map file.
struct NPCDefinition {
    std::string id;                            ///< Stable NPC id referenced by story and cutscenes.
    std::string spriteType;                    ///< Sprite sheet selector.
    std::string name;                          ///< Display name shown in dialogue and battles.
    NPCType type{NPCType::normal};             ///< NPC role classification.
    bool hidden{false};                        ///< Whether the NPC starts hidden.
    std::string dialogueSourcePath;            ///< Optional external dialogue file path.
    Position position{0, 0};                   ///< Initial tile position.
    Direction facing{Direction::down};         ///< Initial facing direction.
    int sightRange{0};                         ///< Trainer detection range in tiles.
    std::vector<DialogueStage> dialogueStages; ///< Dialogue variants selected by flags.
    std::vector<NPCPartyMember> party;         ///< Trainer party members.
};

/// Parsed top-level map definition prior to runtime `Map` construction.
struct MapDefinition {
    std::string id;                            ///< Unique map id.
    int width{0};                              ///< Width in tiles.
    int height{0};                             ///< Height in tiles.
    std::string backgroundImage;               ///< Base tileset image path.
    std::string backgroundImageOverlay;        ///< Overlay image path.
    std::string musicPath;                     ///< Looping background music file path.
    std::optional<Position> playerSpawn;       ///< Optional player spawn tile.
    std::vector<std::string> tileRows;         ///< Raw tile rows from the `[tiles]` section.
    std::vector<WarpPoint> warps;              ///< Parsed warp triggers.
    std::vector<WildEncounterSlot> encounters; ///< Parsed encounter table.
    std::vector<NPCDefinition> npcs;           ///< Parsed NPC definitions.
};

/// Result of parsing a `.map` file.
struct ParseResult {
    MapDefinition definition;          ///< Parsed definition, possibly partial.
    std::vector<std::string> warnings; ///< Non-fatal parse warnings.

    /// Returns whether the parsed definition contains the required header fields.
    bool valid() const {
        return !definition.id.empty() && definition.width > 0 && definition.height > 0;
    }
};

/// Parses a map definition from the current stream position.
ParseResult parse(std::istream &input);

} // namespace MapFormat
