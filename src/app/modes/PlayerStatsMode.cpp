#include "PlayerStatsMode.h"
#include "../../story/StoryManager.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"

void PlayerStatsMode::update(GameContext &ctx, InputManager &input) {
    if (input.isCancelPressed() || input.isConfirmPressed())
        ctx.pushRequest(ModeRequest::changeState(GameState::menu));
}

void PlayerStatsMode::render(GameContext &ctx) {
    const auto objective = ctx.story.currentObjective(ctx.world.getPlayer());
    ctx.ui.drawPlayerStatsScreen(ctx.world.getPlayer(), objective.title, objective.lines);
}
