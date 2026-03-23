/**
 * @file
 * @brief Main overworld mode handling movement, interaction, encounters, and warp flow.
 * @ingroup app_core
 */

#pragma once
#include "../../story/StoryAction.h"
#include "../GameMode.h"

/**
 * @brief Default exploration mode for walking the player around the world.
 * @ingroup app_core
 */
class OverworldMode : public GameMode {
  public:
    /// Runs any story hook associated with entering the current map.
    void onEnter(GameContext &ctx) override;
    /// Updates player movement, interactions, and encounter checks.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the current overworld scene.
    void render(GameContext &ctx) override;

  private:
    // Wild encounter / warp-block / trainer check helpers
    bool wildBattleStarts(GameContext &ctx);
    bool warpBlockStarts(GameContext &ctx);
    bool trainerBattleStarts(GameContext &ctx);
    bool dialogueStarts(GameContext &ctx, NPC *npc);
    void handlePlayerMove(GameContext &ctx, InputManager &input);
    void handlePlayerWarpAttempt(GameContext &ctx);
    void handlePlayerInteraction(GameContext &ctx);

    bool pendingWarpBlock{false}; ///< Whether a blocked warp should trigger after movement settles.
    StoryBlockWarpAction pendingWarpBlockAction; ///< Deferred warp-block action.

    bool justStepped{false}; ///< Whether the player completed a step this frame.

    bool wallHitPlayed{false}; ///< Prevents repeated wall-hit sounds while holding a direction.
    Direction wallHitDir{Direction::down}; ///< Direction tied to the last wall-hit sound.
};
