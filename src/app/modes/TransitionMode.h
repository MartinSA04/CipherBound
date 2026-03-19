/**
 * @file
 * @brief Fade transition mode used for map warps and blackout respawn.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"
#include "../../battle/ui/BattleRenderer.h"
#include <string>

/// Transitional mode that fades between overworld/battle-adjacent states.
class TransitionMode : public GameMode {
  public:
    /// Type of transition behavior to execute.
    enum class Kind {
        mapWarp,         ///< Fade to a different map and spawn.
        blackoutRespawn, ///< Fade to the stored respawn point after defeat.
    };

    /// Creates a map-warp transition.
    TransitionMode(const std::string &targetMapId, const Position &targetSpawn);
    /// Creates a transition using one of the predefined behaviors.
    explicit TransitionMode(Kind kind);

    /// Advances fade timing and completes the transition when needed.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the fade overlay and any required backdrop.
    void render(GameContext &ctx) override;
    /// Performs transition setup when the mode becomes active.
    void onEnter(GameContext &ctx) override;

  private:
    /// Current fade phase.
    enum class Phase {
        fadingOut, ///< Fade overlay is becoming opaque.
        fadingIn,  ///< Fade overlay is becoming transparent.
    };

    void completeFadeOut(GameContext &ctx);

    Kind kind{Kind::mapWarp};            ///< Selected transition behavior.
    Phase phase{Phase::fadingOut};       ///< Active fade phase.
    std::string targetMapId;             ///< Destination map for map-warp transitions.
    Position targetSpawn;                ///< Destination spawn point for map-warp transitions.
    int phaseFrame{0};                   ///< Frame counter within the current phase.
    int phaseDuration{1};                ///< Duration in frames of one fade phase.
    bool renderBattleBackdrop{false};    ///< Whether to draw the battle backdrop during transition.
    BattleRenderer battleRenderer;       ///< Battle backdrop renderer used by some transitions.
};
