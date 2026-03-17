#include "Player.h"

Player::Player(const std::string &name, Position position) : Entity(name, position) {}

void Player::update() {}

void Player::addDaemon(Daemon daemon) {
    markCaught(daemon.getSpeciesId());
    if (partySize() < 6) {
        partyStorage.addDaemon(std::move(daemon));
    } else {
        partyStorage.addDaemonToBox(std::move(daemon));
    }
}

void Player::restorePartyDaemon(Daemon daemon) { partyStorage.addDaemon(std::move(daemon)); }

bool Player::restoreBoxDaemon(int boxIndex, Daemon daemon) {
    return partyStorage.addDaemonToBox(boxIndex, std::move(daemon));
}

Daemon &Player::getDaemon(int index) { return partyStorage.getDaemon(index); }

const Daemon &Player::getDaemon(int index) const { return partyStorage.getDaemon(index); }

const std::vector<Daemon> &Player::getParty() const { return partyStorage.getParty(); }

int Player::partySize() const { return partyStorage.partySize(); }

bool Player::partyEmpty() const { return partyStorage.partyEmpty(); }

void Player::swapDaemon(int indexA, int indexB) { partyStorage.swapDaemon(indexA, indexB); }

void Player::clearParty() { partyStorage.clearParty(); }

void Player::addDaemonToBox(Daemon daemon) { partyStorage.addDaemonToBox(std::move(daemon)); }

void Player::depositDaemon(int partyIndex) { partyStorage.depositDaemon(partyIndex); }

void Player::withdrawDaemon(int boxIndex, int slot) { partyStorage.withdrawDaemon(boxIndex, slot); }

bool Player::canDeposit() const { return partyStorage.canDeposit(); }

bool Player::canWithdraw() const { return partyStorage.canWithdraw(); }

const std::vector<Daemon> &Player::getBox(int boxIndex) const {
    return partyStorage.getBox(boxIndex);
}

int Player::getBoxCount(int boxIndex) const { return partyStorage.getBoxCount(boxIndex); }

int Player::getCurrentBox() const { return partyStorage.getCurrentBox(); }

void Player::setCurrentBox(int box) { partyStorage.setCurrentBox(box); }

void Player::clearPCBoxes() { partyStorage.clearPCBoxes(); }

void Player::addItem(int itemId, int quantity) { inventory.addItem(itemId, quantity); }

bool Player::removeItem(int itemId, int quantity) { return inventory.removeItem(itemId, quantity); }

int Player::getItemCount(int itemId) const { return inventory.getItemCount(itemId); }

const std::vector<InventoryEntry> &Player::getInventory() const { return inventory.getInventory(); }

void Player::clearInventory() { inventory.clearInventory(); }

int Player::getMoney() const { return inventory.getMoney(); }

void Player::addMoney(int amount) { inventory.addMoney(amount); }

bool Player::spendMoney(int amount) { return inventory.spendMoney(amount); }

void Player::setMoney(int amount) { inventory.setMoney(amount); }

void Player::setFlag(const std::string &flag) { eventFlags.setFlag(flag); }

bool Player::hasFlag(const std::string &flag) const { return eventFlags.hasFlag(flag); }

void Player::clearFlag(const std::string &flag) { eventFlags.clearFlag(flag); }

const std::set<std::string> &Player::getFlags() const { return eventFlags.getFlags(); }

void Player::addBadge(const std::string &badgeId) { eventFlags.addBadge(badgeId); }

bool Player::hasBadge(const std::string &badgeId) const { return eventFlags.hasBadge(badgeId); }

int Player::badgeCount() const { return eventFlags.badgeCount(); }

const std::vector<std::string> &Player::getBadges() const { return eventFlags.getBadges(); }

void Player::clearBadges() { eventFlags.clearBadges(); }

void Player::clearFlags() { eventFlags.clearFlags(); }

void Player::markSeen(int speciesId) { daemonDex.markSeen(speciesId); }

void Player::markCaught(int speciesId) { daemonDex.markCaught(speciesId); }

bool Player::hasSeen(int speciesId) const { return daemonDex.hasSeen(speciesId); }

bool Player::hasCaught(int speciesId) const { return daemonDex.hasCaught(speciesId); }

int Player::seenCount() const { return daemonDex.seenCount(); }

int Player::caughtCount() const { return daemonDex.caughtCount(); }

const std::set<int> &Player::getSeenSet() const { return daemonDex.getSeenSet(); }

const std::set<int> &Player::getCaughtSet() const { return daemonDex.getCaughtSet(); }

void Player::clearDaemondex() { daemonDex.clearDaemondex(); }
