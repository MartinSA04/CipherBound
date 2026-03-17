#pragma once

#include <random>
#include <string_view>

class Daemon;
struct ItemData;

struct BattleCaptureOutcome {
    bool success{false};
    int shakes{0};
};

class BattleCapture {
  public:
    static BattleCaptureOutcome resolve(const Daemon &target, const ItemData &ball,
                                        std::mt19937 &rng);
    static Daemon caughtDaemon(const Daemon &target);
    static std::string_view failureMessage(int shakes);
};
