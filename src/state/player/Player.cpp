#include "Player.h"

Player::Player(const std::string &name, Position position) : Entity(name, position) {}

void Player::update() {}

void Player::addDaemon(Daemon daemon) {
    markCaught(daemon.getSpeciesId());
    if (partySize() < 6) {
        party.push_back(std::move(daemon));
    } else {
        addDaemonToBox(daemon);
    }
}