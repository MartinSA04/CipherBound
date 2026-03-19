/**
 * @file
 * @brief Save confirmation mode.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"

/// Mode that performs a save and shows the result to the player.
class SaveMode : public GameMode {
  public:
    /// Advances save flow and exit handling.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders save progress/result UI.
    void render(GameContext &ctx) override;
    /// Starts the save operation when the mode becomes active.
    void onEnter(GameContext &ctx) override;

  private:
    bool saveComplete{false}; ///< Whether the save attempt has finished.
    bool saveSuccess{false};  ///< Whether the save attempt succeeded.
};
