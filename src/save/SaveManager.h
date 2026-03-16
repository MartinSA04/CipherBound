#pragma once
#include "../data/Pokedex.h"
#include "../state/Player.h"
#include "../state/World.h"
#include <string>
#include <vector>

class SaveManager {
  public:
    SaveManager();

    bool saveGame(const std::string &filepath, const Player &player,
                  const World &world);
    bool loadGame(const std::string &filepath, Player &player, World &world,
                  const Pokedex &pokedex);
    bool saveFileExists(const std::string &filepath) const;
    bool deleteSave(const std::string &filepath);

    // Slot-based save system
    std::string getSavePath(int slot) const;
    int getSlotCount() const;

    // Quick info for title screen slot display
    struct SlotInfo {
        bool exists{false};
        std::string playerName;
        int partySize{0};
        int badgeCount{0};
        std::string mapId;
    };
    SlotInfo getSlotInfo(int slot) const;
    std::vector<SlotInfo> getSlotInfos() const;

  private:
    static constexpr int MAX_SAVE_SLOTS = 4;
    std::string baseSavePath;

    // Daemon serialization helpers
    static std::string serializeDaemon(const Daemon &daemon);
    static Daemon deserializeDaemon(const std::string &line,
                                    const Pokedex &pokedex);
    static std::string serializeBaseStats(const BaseStats &stats);
    static BaseStats deserializeBaseStats(const std::string &s);
};
