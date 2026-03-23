/**
 * @file
 * @brief Runtime daemon model including stats, HP, status, EXP, and learned moves.
 * @ingroup world_state
 */

#pragma once
#include "../game_data/Move.h"
#include "../game_data/Species.h"
#include <array>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <vector>

/// Runtime move slot including remaining PP.
struct MoveSlot {
    int moveId;    ///< Move id, or `-1` for an empty slot.
    int currentPP; ///< Current PP.
    int maxPP;     ///< Maximum PP.
};

/// Gen 4 nature table in canonical order.
enum class Nature {
    hardy,
    lonely,
    brave,
    adamant,
    naughty,
    bold,
    docile,
    relaxed,
    impish,
    lax,
    timid,
    hasty,
    serious,
    jolly,
    naive,
    modest,
    mild,
    quiet,
    bashful,
    rash,
    calm,
    gentle,
    sassy,
    careful,
    quirky,
};

/// Detailed information about one resolved level-up.
struct LevelUpResult {
    int oldLevel;        ///< Level before leveling up.
    int newLevel;        ///< Level after leveling up.
    BaseStats statGains; ///< Per-stat increases from the level-up.
};

/**
 * @brief One owned daemon instance with per-save combat state.
 * @ingroup world_state
 */
class Daemon {
  public:
    /// Creates a new daemon from species data and a starting level.
    Daemon(const Species &species, int level);
    /// Creates a fresh daemon with Gen 4-style random IVs/nature and zero EVs.
    static Daemon generateRandomized(const Species &species, int level, std::mt19937 &rng);

    /// Reconstructs a daemon from serialized save data.
    Daemon(const Species &species, int level, int exp, int currentHP, const std::string &nickname,
           StatusEffect status, const BaseStats &ivs, const BaseStats &evs,
           const std::array<MoveSlot, 4> &moves, Nature nature = Nature::hardy);

    /// Returns the current nickname.
    const std::string &getNickname() const;
    /// Sets the nickname.
    void setNickname(const std::string &name);

    /// Returns the current level.
    int getLevel() const;
    /// Returns the total EXP.
    int getExp() const;
    /// Returns the EXP earned since reaching the current level.
    int getExpProgress() const;
    /// Returns the EXP needed for the next level.
    int getExpNeeded() const;
    /// Adds EXP without immediately resolving level-up UI.
    void addExp(int amount);
    /// Awards EVs using Gen 4 caps and returns the EVs that were actually applied.
    BaseStats gainEffortValues(const BaseStats &yield);
    /// Applies pending level-up rules and returns whether a level was gained.
    bool checkLevelUp();
    /// Resolves one pending level-up and returns the resulting stat gains.
    std::optional<LevelUpResult> resolveLevelUp();
    /// Returns move ids learned at the given level.
    std::vector<int> getMovesLearnedAtLevel(int learnedLevel) const;
    /// Returns the species id this daemon can evolve into at its current level, if any.
    std::optional<int> getEvolutionTargetSpeciesId() const;
    /// Replaces this daemon's species data with its evolved form.
    void evolveTo(const Species &species);

    /// Returns the current HP.
    int getCurrentHP() const;
    /// Returns the maximum HP.
    int getMaxHP() const;
    /// Applies damage, clamping at zero.
    void takeDamage(int amount);
    /// Restores HP up to the maximum.
    void heal(int amount);
    /// Fully restores HP and clears status.
    void fullHeal();
    /// Returns whether HP has reached zero.
    bool isFainted() const;

    /// Returns a computed stat by index: 0=hp, 1=atk, 2=def, 3=spa, 4=spd, 5=spe.
    int getStat(int statIndex) const;
    /// Returns the immutable species definition backing this daemon.
    const Species &getSpecies() const;
    /// Returns the current status condition.
    StatusEffect getStatus() const;
    /// Sets the status condition.
    void setStatus(StatusEffect status);
    /// Clears the current status condition.
    void clearStatus();

    /// Returns the move slots.
    const std::array<MoveSlot, 4> &getMoves() const;
    /// Returns whether the daemon already knows the given move.
    bool knowsMove(int moveId) const;
    /// Returns the first empty move slot, or `-1` if all slots are occupied.
    int firstEmptyMoveSlot() const;
    /// Deducts PP from a move slot and returns whether the move could be used.
    bool useMove(int slot);
    /// Learns or replaces a move in the selected slot.
    bool learnMove(int moveId, int slot, int maxPP = 15);

    /// Returns the species id.
    int getSpeciesId() const;

    /// Returns the IV set.
    const BaseStats &getIVs() const;
    /// Returns the EV set.
    const BaseStats &getEVs() const;
    /// Returns the nature.
    Nature getNature() const;

  private:
    std::string nickname;
    int speciesId;
    int level;
    int exp;
    int currentHP;
    StatusEffect status;

    BaseStats ivs; ///< Individual values.
    BaseStats evs; ///< Effort values.
    Nature nature;

    std::array<MoveSlot, 4> moves; ///< Learned move slots.

    int calculateStatAtLevel(int base, int iv, int ev, int statLevel, int statIndex) const;
    int calculateStat(int base, int iv, int ev, int statIndex) const;
    BaseStats calculateStatsForLevel(int statLevel) const;

    std::reference_wrapper<const Species> speciesRef; ///< Referenced species definition.
};
