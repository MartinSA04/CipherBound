#include "DaemonDex.h"

void DaemonDex::markSeen(int speciesId) { seenSpecies.insert(speciesId); }
void DaemonDex::markCaught(int speciesId) {
    seenSpecies.insert(speciesId);
    caughtSpecies.insert(speciesId);
}
bool DaemonDex::hasSeen(int speciesId) const {
    return seenSpecies.count(speciesId) > 0;
}
bool DaemonDex::hasCaught(int speciesId) const {
    return caughtSpecies.count(speciesId) > 0;
}
int DaemonDex::seenCount() const {
    return static_cast<int>(seenSpecies.size());
}
int DaemonDex::caughtCount() const {
    return static_cast<int>(caughtSpecies.size());
}
const std::set<int> &DaemonDex::getSeenSet() const { return seenSpecies; }
const std::set<int> &DaemonDex::getCaughtSet() const { return caughtSpecies; }
void DaemonDex::clearDaemondex() {
    seenSpecies.clear();
    caughtSpecies.clear();
}
