#pragma once
#include <vector>
#include <map>
#include <set>
#include "Entity.h"
#include "Daemon.h"
#include "../data/Item.h"

struct InventoryEntry
{
    int itemId;
    int quantity;
};

class Player : public Entity
{
public:
    Player(const std::string &name, Position position);

    void update() override;

    // Movement
    void move(Direction direction);
    bool canMove(Direction direction, const Map &map) const;
    bool canStep() const;
    bool canTurn() const;
    void startTurnCooldown();
    void setMoveDelay(int frames);
    int getMoveDelay() const;

    // Animation
    void startAnimation();
    int getAnimationFrame();
    void updateAnimation();
    bool isMoving() const;
    bool wasRecentlyMoving() const;
    int getPixelOffsetX() const;
    int getPixelOffsetY() const;
    int getWalkFrame() const;

    // Party management
    void addDaemon(Daemon daemon);
    Daemon &getDaemon(int index);
    const std::vector<Daemon> &getParty() const;
    int partySize() const;
    void swapDaemon(int indexA, int indexB);

    // PC Box storage
    static constexpr int BOX_SIZE = 30;
    static constexpr int NUM_BOXES = 8;
    void depositDaemon(int partyIndex);           // Move from party to current box
    void withdrawDaemon(int boxIndex, int slot);  // Move from box to party
    bool canDeposit() const;                        // Party must keep at least 1
    bool canWithdraw() const;                       // Party must have room (< 6)
    const std::vector<Daemon> &getBox(int boxIndex) const;
    int getBoxCount(int boxIndex) const;
    int getCurrentBox() const;
    void setCurrentBox(int box);

    // Inventory
    void addItem(int itemId, int quantity = 1);
    bool removeItem(int itemId, int quantity = 1);
    int getItemCount(int itemId) const;
    const std::vector<InventoryEntry> &getInventory() const;

    // Currency
    int getMoney() const;
    void addMoney(int amount);
    bool spendMoney(int amount);

    // Badges / progression
    void addBadge(const std::string &badgeId);
    bool hasBadge(const std::string &badgeId) const;
    int badgeCount() const;

    // Event flags for story progression
    void setFlag(const std::string &flag);
    bool hasFlag(const std::string &flag) const;
    void clearFlag(const std::string &flag);
    const std::set<std::string> &getFlags() const;

    // Save/load helpers
    void clearParty();
    void clearInventory();
    void clearBadges();
    void clearFlags();
    void clearPCBoxes();
    void clearDaemondex();
    void setMoney(int amount);
    const std::vector<std::string> &getBadges() const;

    // Daemondex (seen/caught tracking)
    void markSeen(int speciesId);
    void markCaught(int speciesId);
    bool hasSeen(int speciesId) const;
    bool hasCaught(int speciesId) const;
    int seenCount() const;
    int caughtCount() const;
    const std::set<int> &getSeenSet() const;
    const std::set<int> &getCaughtSet() const;

private:
    std::vector<Daemon> party;
    std::vector<std::vector<Daemon>> pcBoxes;  // PC storage boxes
    int currentBox{0};                           // Currently selected box
    std::vector<InventoryEntry> inventory;
    std::vector<std::string> badges;
    std::set<std::string> eventFlags;
    std::set<int> seenSpecies;
    std::set<int> caughtSpecies;
    int money;

    int moveDelay;       // frames per tile movement (animation duration)
    int pixelOffsetX;    // current pixel offset during animation
    int pixelOffsetY;
    int animFramesLeft;  // remaining animation frames
    int turnCooldown;    // frames to wait after turning before moving
    int walkFrame;       // walk cycle counter (alternates feet each step)
    bool wasMoving;      // was the player walking last frame (for turn cooldown logic)
};
