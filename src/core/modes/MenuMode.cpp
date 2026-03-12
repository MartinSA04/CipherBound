#include "MenuMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../ui/Renderer.h"
#include "../../ui/SpriteFont.h"
#include "../../audio/SoundManager.h"

void MenuMode::update(GameContext &ctx, InputManager &input)
{
    ctx.ui.navigateVertical(selected, 5);

    if (input.isCancelPressed())
    {
        ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
    }

    if (input.isConfirmPressed())
    {
        ctx.playSound(SoundEffect::select);
        switch (selected)
        {
        case 0:
            ctx.pushRequest(ModeRequest::changeState(GameState::daemondex));
            break;
        case 1:
            ctx.pushRequest(ModeRequest::changeState(GameState::party));
            break;
        case 2:
            ctx.pushRequest(ModeRequest::changeState(GameState::bag));
            break;
        case 3:
            ctx.pushRequest(ModeRequest::changeState(GameState::saving));
            break;
        case 4:
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

    static const std::vector<std::string> menuItems = {"Daemondex", "Daemons", "Bag", "Save", "Exit"};
    int scale = PIXEL_SCALE;

    int maxTextW = 0;
    for (const auto &opt : menuItems)
    {
        int tw = spriteFont.getTextWidth(opt, scale);
        if (tw > maxTextW) maxTextW = tw;
    }

    int itemHeight = 18 * scale + 6;
    int menuWidth = maxTextW + 10 * scale;
    int menuX = WINDOW_WIDTH - menuWidth - 10;
    int menuY = 10;
    int menuHeight = static_cast<int>(menuItems.size()) * itemHeight + 16;

    renderer.drawFilledRect(menuX, menuY, menuWidth, menuHeight, TDT4102::Color{240, 245, 255});
    renderer.drawRect(menuX, menuY, menuWidth, menuHeight,
                      TDT4102::Color::transparent, TDT4102::Color{60, 70, 100});

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
