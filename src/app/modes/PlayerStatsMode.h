/**
 * @file
 * @brief Trainer stats overview mode.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"

/// Mode for viewing player-facing progress stats such as money and badges.
class PlayerStatsMode : public GameMode {
  public:
    /// Handles returning to the pause menu.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the stats overview.
    void render(GameContext &ctx) override;
};
