#include "MenuMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"

void MenuMode::update(GameContext &ctx, InputManager &input)
{
    ctx.ui.navigateVertical(selected, 4);

    if (input.isCancelPressed())
    {
        ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
    }

    if (input.isConfirmPressed())
    {
        switch (selected)
        {
        case 0:
            ctx.pushRequest(ModeRequest::changeState(GameState::party));
            break;
        case 1:
            ctx.pushRequest(ModeRequest::changeState(GameState::bag));
            break;
        case 2:
            ctx.pushRequest(ModeRequest::changeState(GameState::saving));
            break;
        case 3:
            ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
            break;
        }
    }
}

void MenuMode::render(GameContext &ctx)
{
    renderOverworld(ctx);
    ctx.ui.drawMainMenu(selected);
}
