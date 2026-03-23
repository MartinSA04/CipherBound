/**
 * @file
 * @brief Helpers for move validation and attack resolution within battles.
 * @ingroup battle_system
 */

#pragma once

#include <random>

class Daemon;
class Pokedex;
struct MoveData;

/// Error codes returned when preparing a move selection.
enum class BattleMoveSelectionError {
    none,             ///< Selection is valid.
    invalidSelection, ///< Requested slot is out of range or empty.
    noPP,             ///< The slot exists but cannot currently be used.
};

/// Result of validating a selected move slot.
struct BattleMoveSelection {
    BattleMoveSelectionError error{BattleMoveSelectionError::none}; ///< Validation outcome.
    int slot{-1};                  ///< Resolved slot index, or `-1` on failure.
    const MoveData *move{nullptr}; ///< Selected move data, if validation succeeded.
};

/// Result of executing one attack.
struct BattleAttackResolution {
    bool hit{false};             ///< Whether the attack connected.
    int damage{0};               ///< Damage dealt to the defender.
    float effectiveness{1.0f};   ///< Effectiveness multiplier that was applied.
    bool defenderFainted{false}; ///< Whether the defender fainted as a result.
};

/**
 * @brief Static helpers for validating selected moves and resolving attacks.
 * @ingroup battle_system
 */
class BattleTurnResolver {
  public:
    /// Validates the player's selected move slot.
    static BattleMoveSelection preparePlayerMove(Daemon &playerDaemon, int moveSlot,
                                                 const Pokedex &pokedex);
    /// Chooses and validates the opponent's move slot.
    static BattleMoveSelection prepareOpponentMove(Daemon &opponentDaemon,
                                                   const Daemon &playerDaemon,
                                                   const Pokedex &pokedex, std::mt19937 &rng);
    /// Resolves one attack from attacker to defender using the supplied move.
    static BattleAttackResolution resolveAttack(Daemon &attacker, Daemon &defender,
                                                const MoveData &move, std::mt19937 &rng);
};
