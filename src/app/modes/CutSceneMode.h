/**
 * @file
 * @brief Mode that delegates frame updates to the cutscene runner.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"

/// Mode that plays a scripted cutscene over the overworld backdrop.
class CutSceneMode : public GameMode {
  public:
    /// Advances cutscene playback and dispatches any follow-up request.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the overworld plus any active cutscene dialogue overlay.
    void render(GameContext &ctx) override;
};
