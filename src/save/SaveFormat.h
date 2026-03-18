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

namespace SaveFormat {

struct SavedInventoryEntry {
    int itemId{0};
    int quantity{0};
};

struct SavedNPCState {
    std::string mapId;
    std::string npcId;
    bool defeated{false};
};

struct SavedDaemonData {
    int speciesId{0};
    int level{0};
    int exp{0};
    int currentHP{0};
    std::string nickname;
    StatusEffect status{StatusEffect::none};
    BaseStats ivs{0, 0, 0, 0, 0, 0};
    BaseStats evs{0, 0, 0, 0, 0, 0};
    std::array<MoveSlot, 4> moves{{{-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}}};
};

struct SaveFileData {
    std::string playerName;
    std::string mapId;
    Position position{0, 0};
    Direction facing{Direction::down};
    int money{0};
    std::string respawnMapId;
    std::optional<Position> respawnPosition;
    std::optional<Direction> respawnFacing;
    std::vector<std::string> flags;
    std::vector<std::string> badges;
    std::vector<SavedInventoryEntry> inventory;
    std::vector<SavedDaemonData> party;
    int currentBox{0};
    std::vector<std::vector<SavedDaemonData>> pcBoxes;
    std::vector<SavedNPCState> npcStates;
    std::set<int> seen;
    std::set<int> caught;
};

struct SaveSlotSummary {
    std::string playerName;
    int partySize{0};
    int badgeCount{0};
    std::string mapId;
};

struct SaveParseResult {
    SaveFileData data;
    std::vector<std::string> warnings;
};

std::string serializeDaemon(const Daemon &daemon);
SaveParseResult parse(std::istream &input);
SaveSlotSummary summarize(const SaveFileData &data);
Daemon hydrateDaemon(const SavedDaemonData &saved, const Pokedex &pokedex);

} // namespace SaveFormat
