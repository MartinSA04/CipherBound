#include "Player.h"
#include "../ui/Renderer.h"
#include <algorithm>

Player::Player(const std::string &name, Position position)
    : Entity(name, position), money(0), moveDelay(12),
      pixelOffsetX(0), pixelOffsetY(0), animFramesLeft(0), turnCooldown(0),
      walkFrame(0), wasMoving(false),
      pcBoxes(NUM_BOXES)
{
}

void Player::update()
{
    updateAnimation();
}

bool Player::canStep() const
{
    return animFramesLeft <= 0 && turnCooldown <= 0;
}

bool Player::canTurn() const
{
    return animFramesLeft <= 0;
}

void Player::startTurnCooldown()
{
    turnCooldown = 7; // Number of frames to wait after turning before allowing movement
}

void Player::setMoveDelay(int frames)
{
    moveDelay = frames;
}

int Player::getMoveDelay() const
{
    return moveDelay;
}

int Player::getAnimationFrame()
{
    return animFramesLeft;
}

void Player::startAnimation()
{
    animFramesLeft = moveDelay;
}

void Player::updateAnimation()
{
    if (turnCooldown > 0)
        --turnCooldown;

    if (animFramesLeft <= 0)
    {
        wasMoving = false;
        return;
    }

    wasMoving = true;
    --animFramesLeft;

    if (animFramesLeft == moveDelay / 2)
        walkFrame++;

    if (animFramesLeft <= 0)
    {
        // Snap to grid
        pixelOffsetX = 0;
        pixelOffsetY = 0;
    }
    else
    {
        // Linearly interpolate: offset goes from full tile toward 0
        // Total pixels to cover = TILE_SIZE, over moveDelay frames
        double t = static_cast<double>(animFramesLeft) / moveDelay;
        int totalOffset = static_cast<int>(t * TILE_SIZE);

        switch (facing)
        {
        case Direction::up:
            pixelOffsetX = 0;
            pixelOffsetY = totalOffset;
            break;
        case Direction::down:
            pixelOffsetX = 0;
            pixelOffsetY = -totalOffset;
            break;
        case Direction::left:
            pixelOffsetX = totalOffset;
            pixelOffsetY = 0;
            break;
        case Direction::right:
            pixelOffsetX = -totalOffset;
            pixelOffsetY = 0;
            break;
        }
    }
}

bool Player::isMoving() const
{
    return animFramesLeft > 0;
}

bool Player::wasRecentlyMoving() const
{
    return wasMoving;
}

int Player::getPixelOffsetX() const { return pixelOffsetX; }
int Player::getPixelOffsetY() const { return pixelOffsetY; }
int Player::getWalkFrame() const { return walkFrame; }
void Player::resetWalkFrame()
{
    // if (not isMoving())
    //     walkFrame = 0;
}

void Player::move(Direction direction)
{
    setFacing(direction);
    // Update tile position immediately (for logic/collision)
    position.moveDirection(direction);

    // Start animation from previous position toward new position
    startAnimation();
    walkFrame++; // alternate feet each step
    switch (direction)
    {
    case Direction::up:
        pixelOffsetX = 0;
        pixelOffsetY = TILE_SIZE;
        break;
    case Direction::down:
        pixelOffsetX = 0;
        pixelOffsetY = -TILE_SIZE;
        break;
    case Direction::left:
        pixelOffsetX = TILE_SIZE;
        pixelOffsetY = 0;
        break;
    case Direction::right:
        pixelOffsetX = -TILE_SIZE;
        pixelOffsetY = 0;
        break;
    }
}

bool Player::canMove(Direction direction, const Map &map) const
{
    Position target = position;
    switch (direction)
    {
    case Direction::up:
        target.y -= 1;
        break;
    case Direction::down:
        target.y += 1;
        break;
    case Direction::left:
        target.x -= 1;
        break;
    case Direction::right:
        target.x += 1;
        break;
    }
    return map.isWalkable(target, direction);
}

// --- Party ---

void Player::addDaemon(Daemon daemon)
{
    if (party.size() < 6)
    {
        party.push_back(std::move(daemon));
    }
    else
    {
        // Party full — send to first available PC box slot
        for (auto &box : pcBoxes)
        {
            if (static_cast<int>(box.size()) < BOX_SIZE)
            {
                box.push_back(std::move(daemon));
                return;
            }
        }
        // All boxes full — Daemon is lost (shouldn't happen with 240 slots)
    }
}

Daemon &Player::getDaemon(int index)
{
    return party.at(index);
}

const std::vector<Daemon> &Player::getParty() const { return party; }
int Player::partySize() const { return static_cast<int>(party.size()); }

void Player::swapDaemon(int indexA, int indexB)
{
    if (indexA >= 0 && indexA < partySize() && indexB >= 0 && indexB < partySize())
        std::swap(party[indexA], party[indexB]);
}

// --- PC Box storage ---

void Player::depositDaemon(int partyIndex)
{
    if (!canDeposit() || partyIndex < 0 || partyIndex >= partySize())
        return;
    if (static_cast<int>(pcBoxes[currentBox].size()) >= BOX_SIZE)
        return;

    pcBoxes[currentBox].push_back(std::move(party[partyIndex]));
    party.erase(party.begin() + partyIndex);
}

void Player::withdrawDaemon(int boxIndex, int slot)
{
    if (!canWithdraw())
        return;
    if (boxIndex < 0 || boxIndex >= NUM_BOXES)
        return;
    if (slot < 0 || slot >= static_cast<int>(pcBoxes[boxIndex].size()))
        return;

    party.push_back(std::move(pcBoxes[boxIndex][slot]));
    pcBoxes[boxIndex].erase(pcBoxes[boxIndex].begin() + slot);
}

bool Player::canDeposit() const
{
    return partySize() > 1; // must keep at least 1
}

bool Player::canWithdraw() const
{
    return partySize() < 6;
}

const std::vector<Daemon> &Player::getBox(int boxIndex) const
{
    return pcBoxes.at(boxIndex);
}

int Player::getBoxCount(int boxIndex) const
{
    return static_cast<int>(pcBoxes.at(boxIndex).size());
}

int Player::getCurrentBox() const { return currentBox; }
void Player::setCurrentBox(int box)
{
    if (box >= 0 && box < NUM_BOXES)
        currentBox = box;
}

// --- Inventory ---

void Player::addItem(int itemId, int quantity)
{
    for (auto &entry : inventory)
    {
        if (entry.itemId == itemId)
        {
            entry.quantity += quantity;
            return;
        }
    }
    inventory.push_back({itemId, quantity});
}

bool Player::removeItem(int itemId, int quantity)
{
    for (auto it = inventory.begin(); it != inventory.end(); ++it)
    {
        if (it->itemId == itemId)
        {
            if (it->quantity < quantity)
                return false;
            it->quantity -= quantity;
            if (it->quantity <= 0)
                inventory.erase(it);
            return true;
        }
    }
    return false;
}

int Player::getItemCount(int itemId) const
{
    for (const auto &entry : inventory)
    {
        if (entry.itemId == itemId)
            return entry.quantity;
    }
    return 0;
}

const std::vector<InventoryEntry> &Player::getInventory() const { return inventory; }

// --- Money ---

int Player::getMoney() const { return money; }

void Player::addMoney(int amount) { money += amount; }

bool Player::spendMoney(int amount)
{
    if (money < amount)
        return false;
    money -= amount;
    return true;
}

// --- Badges ---

void Player::addBadge(const std::string &badgeId)
{
    if (!hasBadge(badgeId))
        badges.push_back(badgeId);
}

bool Player::hasBadge(const std::string &badgeId) const
{
    return std::find(badges.begin(), badges.end(), badgeId) != badges.end();
}

int Player::badgeCount() const { return static_cast<int>(badges.size()); }

// --- Event flags ---

void Player::setFlag(const std::string &flag) { eventFlags.insert(flag); }
bool Player::hasFlag(const std::string &flag) const { return eventFlags.count(flag) > 0; }
void Player::clearFlag(const std::string &flag) { eventFlags.erase(flag); }
const std::set<std::string> &Player::getFlags() const { return eventFlags; }

// --- Save/load helpers ---

void Player::clearParty() { party.clear(); }
void Player::clearInventory() { inventory.clear(); }
void Player::clearBadges() { badges.clear(); }
void Player::clearFlags() { eventFlags.clear(); }
void Player::clearPCBoxes()
{
    for (auto &box : pcBoxes)
        box.clear();
}
void Player::setMoney(int amount) { money = amount; }
const std::vector<std::string> &Player::getBadges() const { return badges; }
