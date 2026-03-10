#include "MenuMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../ui/Renderer.h"
#include "../../ui/SpriteFont.h"

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

    // Draw main menu overlay on right side
    Renderer &renderer = ctx.ui.getRenderer();
    SpriteFont &spriteFont = ctx.ui.getSpriteFont();

    static const std::vector<std::string> menuItems = {"Daemons", "Bag", "Save", "Exit"};
    int scale = PIXEL_SCALE;
    int itemHeight = 18 * scale + 6;
    int menuWidth = 40 * scale;
    int menuX = WINDOW_WIDTH - menuWidth - 10;
    int menuY = 10;
    int menuHeight = static_cast<int>(menuItems.size()) * itemHeight + 16;

    renderer.drawRect(menuX, menuY, menuWidth, menuHeight,
                      TDT4102::Color::white, TDT4102::Color::black);

    for (int i = 0; i < static_cast<int>(menuItems.size()); ++i)
    {
        int oy = menuY + 8 + i * itemHeight;

        if (i == selected)
        {
            ctx.ui.drawSelectionArrow(menuX + 2 * scale, oy + 4 * scale, scale);
        }

        spriteFont.drawText(renderer, menuItems[i], menuX + 6 * scale, oy, scale);
    }
}
