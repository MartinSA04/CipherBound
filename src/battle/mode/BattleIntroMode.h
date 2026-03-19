/**
 * @file
 * @brief Battle intro mode for wild and trainer battle setup.
 * @ingroup battle_system
 */

#pragma once
#include "../../app/GameMode.h"
class NPC;

/// Mode that creates and starts a battle before handing off to `BattleMode`.
class BattleIntroMode : public GameMode {
  public:
    /// Creates a wild battle intro.
    BattleIntroMode(int speciesId, int level);
    /// Creates a trainer battle intro.
    BattleIntroMode(NPC *trainer);

    /// Updates intro animation progress and battle creation.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the intro presentation.
    void render(GameContext &ctx) override;

  private:
    int speciesId{0};   ///< Wild species id for wild battles.
    int level{0};       ///< Wild level for wild battles.
    NPC *trainer{nullptr}; ///< Non-owning trainer pointer for trainer battles.
};
