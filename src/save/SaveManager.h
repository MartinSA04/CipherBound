/**
 * @file
 * @brief Slot-oriented save/load orchestration for the runtime world and player state.
 * @ingroup save_system
 */

#pragma once
#include "../game_data/Pokedex.h"
#include "../state/World.h"
#include "../state/player/Player.h"
#include <string>
#include <vector>

/**
 * @brief Loads, saves, deletes, and summarizes slot-based save files.
 * @ingroup save_system
 */
class SaveManager {
  public:
    /// Creates a save manager using the default save directory.
    SaveManager();

    /// Serializes the current player and world state to the given file path.
    bool saveGame(const std::string &filepath, const Player &player, const World &world);
    /// Loads a save file into the supplied player and world objects.
    bool loadGame(const std::string &filepath, Player &player, World &world,
                  const Pokedex &pokedex);
    /// Returns whether the given save file exists.
    bool saveFileExists(const std::string &filepath) const;
    /// Deletes the given save file if it exists.
    bool deleteSave(const std::string &filepath);

    /// Returns the file path used for a slot index.
    std::string getSavePath(int slot) const;
    /// Returns the number of available save slots.
    int getSlotCount() const;

    /// Lightweight metadata used by the title screen save-slot picker.
    struct SlotInfo {
        bool exists{false};      ///< Whether a valid save file exists for the slot.
        std::string playerName;  ///< Saved player name.
        int partySize{0};        ///< Number of daemons in the active party.
        int badgeCount{0};       ///< Number of earned badges.
        std::string mapId;       ///< Map id stored in the save.
    };
    /// Returns summary information for one slot.
    SlotInfo getSlotInfo(int slot) const;
    /// Returns summary information for all slots.
    std::vector<SlotInfo> getSlotInfos() const;

  private:
    static constexpr int MAX_SAVE_SLOTS = 4;
    std::string baseSavePath;
};
