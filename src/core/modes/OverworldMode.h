#pragma once
#include "../GameMode.h"

class OverworldMode : public GameMode
{
public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

private:
    // Wild encounter / warp-block / trainer check helpers
    bool wildBattleStarts(GameContext &ctx);
    bool warpBlockStarts(GameContext &ctx);
    bool trainerBattleStarts(GameContext &ctx);
    bool dialogueStarts(GameContext &ctx, std::shared_ptr<NPC> npc);
    void handlePlayerMove(GameContext &ctx, InputManager &input);
    void handlePlayerWarpAttempt(GameContext &ctx);
    void handlePlayerInteraction(GameContext &ctx);

    // Pending warp block (deferred until walk animation finishes)
    bool pendingWarpBlock{false};
    StoryAction pendingWarpBlockAction;

    // Wild encounters are only checked once per step.
    // Set to true when the player finishes a step; cleared after a check.
    bool justStepped{false};
};
