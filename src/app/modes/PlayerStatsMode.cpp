#include "PlayerStatsMode.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"

void PlayerStatsMode::update(GameContext &ctx, InputManager &input) {
    if (input.isCancelPressed() || input.isConfirmPressed())
        ctx.pushRequest(ModeRequest::changeState(GameState::menu));
}

void PlayerStatsMode::render(GameContext &ctx) {
    ctx.ui.drawPlayerStatsScreen(ctx.world.getPlayer());
}
