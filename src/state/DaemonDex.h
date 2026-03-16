#pragma once
#include <set>

class DaemonDex
{

public:
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
    std::set<int> seenSpecies;
    std::set<int> caughtSpecies;
};
