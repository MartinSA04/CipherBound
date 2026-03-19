/**
 * @file
 * @brief PC storage box mode.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"
#include <string>

/// Mode for moving daemons between the active party and PC storage boxes.
class PCBoxMode : public GameMode {
  public:
    /// Updates PC box navigation and transfer actions.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the PC box UI.
    void render(GameContext &ctx) override;
    /// Resets view state when entering the mode.
    void onEnter(GameContext &ctx) override;

  private:
    int selected{0};          ///< Current selection index.
    bool viewingParty{true};  ///< Whether the party column is active.
    bool showingMessage{false}; ///< Whether a transient message is shown.
    std::string message;      ///< Active transient message text.
    bool lrHeld{false};       ///< Whether left/right is currently held for box switching.
};
