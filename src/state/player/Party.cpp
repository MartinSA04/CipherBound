#include "Party.h"

void Party::addDaemon(Daemon daemon) { party.push_back(std::move(daemon)); }

Daemon &Party::getDaemon(int index) { return party.at(static_cast<std::size_t>(index)); }

const Daemon &Party::getDaemon(int index) const {
    return party.at(static_cast<std::size_t>(index));
}

const std::vector<Daemon> &Party::getParty() const { return party; }
int Party::partySize() const { return static_cast<int>(party.size()); }
bool Party::partyEmpty() const { return party.empty(); }

void Party::swapDaemon(int indexA, int indexB) {
    if (indexA >= 0 && indexA < partySize() && indexB >= 0 && indexB < partySize())
        std::swap(party[static_cast<std::size_t>(indexA)], party[static_cast<std::size_t>(indexB)]);
}

void Party::clearParty() { party.clear(); }

// --- PC Box storage ---

void PartyAndPCBoxes::addDaemonToBox(Daemon daemon) {
    for (auto &box : pcBoxes) {
        if (box.size() < BOX_SIZE) {
            box.push_back(std::move(daemon));
            return;
        }
    }
}

void PartyAndPCBoxes::depositDaemon(int partyIndex) {
    if (!canDeposit() || partyIndex < 0 || partyIndex >= partySize())
        return;
    if (static_cast<int>(pcBoxes[static_cast<std::size_t>(currentBox)].size()) >= BOX_SIZE)
        return;

    pcBoxes[static_cast<std::size_t>(currentBox)].push_back(
        std::move(party[static_cast<std::size_t>(partyIndex)]));
    party.erase(party.begin() + partyIndex);
}

void PartyAndPCBoxes::withdrawDaemon(int boxIndex, int slot) {
    if (!canWithdraw())
        return;
    if (boxIndex < 0 || boxIndex >= NUM_BOXES)
        return;
    if (slot < 0 || slot >= static_cast<int>(pcBoxes[static_cast<std::size_t>(boxIndex)].size()))
        return;

    party.push_back(
        std::move(pcBoxes[static_cast<std::size_t>(boxIndex)][static_cast<std::size_t>(slot)]));
    pcBoxes[static_cast<std::size_t>(boxIndex)].erase(
        pcBoxes[static_cast<std::size_t>(boxIndex)].begin() + slot);
}

bool PartyAndPCBoxes::canDeposit() const {
    return partySize() > 1; // must keep at least 1
}

bool PartyAndPCBoxes::canWithdraw() const { return partySize() < 6; }

const std::vector<Daemon> &PartyAndPCBoxes::getBox(int boxIndex) const {
    return pcBoxes.at(static_cast<std::size_t>(boxIndex));
}

int PartyAndPCBoxes::getBoxCount(int boxIndex) const {
    return static_cast<int>(pcBoxes.at(static_cast<std::size_t>(boxIndex)).size());
}

int PartyAndPCBoxes::getCurrentBox() const { return currentBox; }
void PartyAndPCBoxes::setCurrentBox(int box) {
    if (box < 0)
        return;
    if (box < NUM_BOXES)
        currentBox = box;
}

void PartyAndPCBoxes::clearPCBoxes() {
    for (auto &box : pcBoxes)
        box.clear();
}
