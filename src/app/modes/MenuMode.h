/**
 * @file
 * @brief Pause menu mode.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"

/// Pause menu mode for opening party, bag, save, and related screens.
class MenuMode : public GameMode {
  public:
    /// Updates menu navigation and request dispatch.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the pause menu.
    void render(GameContext &ctx) override;

  private:
    int selected{0}; ///< Current menu selection index.
};
