#pragma once

#include "../Daemon.h"
#include "../Entity.h"
#include "DaemonDex.h"
#include "EventFlags.h"
#include "Inventory.h"
#include "Party.h"

class Player : public Entity {
  public:
    static constexpr int BOX_SIZE = PartyAndPCBoxes::BOX_SIZE;
    static constexpr int NUM_BOXES = PartyAndPCBoxes::NUM_BOXES;

    Player(const std::string &name, Position position);

    void update() override;

    void addDaemon(Daemon daemon);

    Daemon &getDaemon(int index);
    const Daemon &getDaemon(int index) const;
    const std::vector<Daemon> &getParty() const;
    int partySize() const;
    bool partyEmpty() const;
    void swapDaemon(int indexA, int indexB);
    void clearParty();

    void addDaemonToBox(Daemon daemon);
    void depositDaemon(int partyIndex);
    void withdrawDaemon(int boxIndex, int slot);
    bool canDeposit() const;
    bool canWithdraw() const;
    const std::vector<Daemon> &getBox(int boxIndex) const;
    int getBoxCount(int boxIndex) const;
    int getCurrentBox() const;
    void setCurrentBox(int box);
    void clearPCBoxes();

    void addItem(int itemId, int quantity = 1);
    bool removeItem(int itemId, int quantity = 1);
    int getItemCount(int itemId) const;
    const std::vector<InventoryEntry> &getInventory() const;
    void clearInventory();
    int getMoney() const;
    void addMoney(int amount);
    bool spendMoney(int amount);
    void setMoney(int amount);

    void setFlag(const std::string &flag);
    bool hasFlag(const std::string &flag) const;
    void clearFlag(const std::string &flag);
    const std::set<std::string> &getFlags() const;
    void addBadge(const std::string &badgeId);
    bool hasBadge(const std::string &badgeId) const;
    int badgeCount() const;
    const std::vector<std::string> &getBadges() const;
    void clearBadges();
    void clearFlags();

    void markSeen(int speciesId);
    void markCaught(int speciesId);
    bool hasSeen(int speciesId) const;
    bool hasCaught(int speciesId) const;
    int seenCount() const;
    int caughtCount() const;
    const std::set<int> &getSeenSet() const;
    const std::set<int> &getCaughtSet() const;
    void clearDaemondex();

  private:
    Inventory inventory;
    EventFlags eventFlags;
    PartyAndPCBoxes partyStorage;
    DaemonDex daemonDex;
};
