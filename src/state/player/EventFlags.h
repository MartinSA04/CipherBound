#pragma once
#include <set>
#include <string>
#include <vector>

class EventFlags {
    std::vector<std::string> badges;
    std::set<std::string> eventFlags;

  public:
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
};
