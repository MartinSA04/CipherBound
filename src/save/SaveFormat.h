/**
 * @file
 * @brief Parsed schema and helpers for CipherBound save files.
 * @ingroup save_system
 * @ingroup data_formats
 */

#pragma once

#include "../game_data/Move.h"
#include "../game_data/Species.h"
#include "../state/Daemon.h"
#include "../state/Movement.h"
#include <array>
#include <iosfwd>
#include <optional>
#include <set>
#include <string>
#include <vector>

class Pokedex;

/**
 * @brief Save-file parsing and serialization helpers.
 * @ingroup save_system
 */
namespace SaveFormat {

/// Serializable inventory entry inside a save file.
struct SavedInventoryEntry {
    int itemId{0};   ///< Item id from the game-data catalog.
    int quantity{0}; ///< Number of items in the stack.
};

/// Saved defeated-state entry for one NPC.
struct SavedNPCState {
    std::string mapId;    ///< Map containing the NPC.
    std::string npcId;    ///< Stable NPC id within that map.
    bool defeated{false}; ///< Whether the NPC has already been defeated.
};

/// Serialized daemon snapshot used by party and PC box sections.
struct SavedDaemonData {
    int speciesId{0};                        ///< Species id for the daemon.
    int level{0};                            ///< Current daemon level.
    int exp{0};                              ///< Current experience total.
    int currentHP{0};                        ///< Current HP when the game was saved.
    std::string nickname;                    ///< Player-visible nickname.
    StatusEffect status{StatusEffect::none}; ///< Persistent status condition.
    BaseStats ivs{0, 0, 0, 0, 0, 0};         ///< Individual values.
    BaseStats evs{0, 0, 0, 0, 0, 0};         ///< Effort values.
    Nature nature{Nature::hardy};            ///< Gen 4 nature affecting non-HP stats.
    std::array<MoveSlot, 4> moves{
        {{-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}}}; ///< Move slots with PP state.
};

/// Complete save-file payload after parsing.
struct SaveFileData {
    std::string playerName;                     ///< Saved player name.
    std::string mapId;                          ///< Current map id.
    Position position{0, 0};                    ///< Player tile on the current map.
    Direction facing{Direction::down};          ///< Player facing direction.
    int money{0};                               ///< Current money total.
    std::string respawnMapId;                   ///< Default blackout respawn map.
    std::optional<Position> respawnPosition;    ///< Optional blackout respawn position.
    std::optional<Direction> respawnFacing;     ///< Optional blackout respawn facing.
    std::vector<std::string> flags;             ///< Set story/event flags, stored as strings.
    std::vector<std::string> badges;            ///< Earned badge ids.
    std::vector<SavedInventoryEntry> inventory; ///< Bag contents.
    std::vector<SavedDaemonData> party;         ///< Active party daemons.
    int currentBox{0};                          ///< Currently selected PC box index.
    std::vector<std::vector<SavedDaemonData>> pcBoxes; ///< Stored daemons grouped by box.
    std::vector<SavedNPCState> npcStates;              ///< Saved NPC defeated states.
    std::set<int> seen;                                ///< Seen species ids.
    std::set<int> caught;                              ///< Caught species ids.
};

/// Reduced save summary for slot-selection UI.
struct SaveSlotSummary {
    std::string playerName; ///< Saved player name.
    int partySize{0};       ///< Number of daemons in the active party.
    int badgeCount{0};      ///< Number of earned badges.
    std::string mapId;      ///< Current map id.
};

/// Result of parsing a save file, including non-fatal warnings.
struct SaveParseResult {
    SaveFileData data;                 ///< Parsed data, possibly partial.
    std::vector<std::string> warnings; ///< Non-fatal parse warnings.
};

/// Serializes one daemon into the compact line format used in save sections.
std::string serializeDaemon(const Daemon &daemon);
/// Parses a save file from the current stream position.
SaveParseResult parse(std::istream &input);
/// Builds a slot summary suitable for menus and title screens.
SaveSlotSummary summarize(const SaveFileData &data);
/// Reconstructs a runtime daemon object from saved data and the pokedex catalog.
Daemon hydrateDaemon(const SavedDaemonData &saved, const Pokedex &pokedex);

} // namespace SaveFormat
