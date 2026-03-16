#include "EventFlags.h"
#include <algorithm>

// --- Badges ---

void EventFlags::addBadge(const std::string &badgeId)
{
    if (!hasBadge(badgeId))
        badges.push_back(badgeId);
}

bool EventFlags::hasBadge(const std::string &badgeId) const
{
    return std::find(badges.begin(), badges.end(), badgeId) != badges.end();
}

int EventFlags::badgeCount() const { return static_cast<int>(badges.size()); }
const std::vector<std::string> &EventFlags::getBadges() const { return badges; }

// --- Event flags ---

void EventFlags::setFlag(const std::string &flag) { eventFlags.insert(flag); }
bool EventFlags::hasFlag(const std::string &flag) const { return eventFlags.count(flag) > 0; }
void EventFlags::clearFlag(const std::string &flag) { eventFlags.erase(flag); }
const std::set<std::string> &EventFlags::getFlags() const { return eventFlags; }

void EventFlags::clearBadges() { badges.clear(); }
void EventFlags::clearFlags() { eventFlags.clear(); }