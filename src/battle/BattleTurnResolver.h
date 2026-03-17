#pragma once

#include <random>

class Daemon;
class Pokedex;
struct MoveData;

enum class BattleMoveSelectionError {
    none,
    invalidSelection,
    noPP,
};

struct BattleMoveSelection {
    BattleMoveSelectionError error{BattleMoveSelectionError::none};
    int slot{-1};
    const MoveData *move{nullptr};
};

struct BattleAttackResolution {
    bool hit{false};
    int damage{0};
    float effectiveness{1.0f};
    bool defenderFainted{false};
};

class BattleTurnResolver {
  public:
    static BattleMoveSelection preparePlayerMove(Daemon &playerDaemon, int moveSlot,
                                                 const Pokedex &pokedex);
    static BattleMoveSelection prepareOpponentMove(Daemon &opponentDaemon,
                                                   const Daemon &playerDaemon,
                                                   const Pokedex &pokedex, std::mt19937 &rng);
    static BattleAttackResolution resolveAttack(Daemon &attacker, Daemon &defender,
                                                const MoveData &move, std::mt19937 &rng);
};
