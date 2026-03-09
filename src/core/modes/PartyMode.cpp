#include "PartyMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../world/World.h"
#include "../../world/Player.h"

void PartyMode::update(GameContext &ctx, InputManager &input)
{
    ctx.ui.navigateVertical(selected, ctx.world.getPlayer().partySize());

    if (input.isCancelPressed())
    {
        ctx.pushRequest(ModeRequest::changeState(GameState::menu));
    }
}

void PartyMode::render(GameContext &ctx)
{
    ctx.ui.drawPartyList(ctx.world.getPlayer(), selected);
}
