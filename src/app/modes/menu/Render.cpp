#include "../../../ui/GameUI.h"
#include "../../../ui/Renderer.h"
#include "../../../ui/SpriteFont.h"
#include "../MenuMode.h"

void MenuMode::render(GameContext &ctx) {
    renderOverworld(ctx);

    Renderer &renderer = ctx.ui.getRenderer();
    SpriteFont &spriteFont = ctx.ui.getSpriteFont();

    static const std::vector<std::string> menuItems = {"Daemondex", "Stats", "Daemons",
                                                       "Bag",       "Save",  "Exit"};
    int scale = PIXEL_SCALE;

    int maxTextW = 0;
    for (const auto &opt : menuItems) {
        int tw = spriteFont.getTextWidth(opt, scale);
        if (tw > maxTextW)
            maxTextW = tw;
    }

    int itemHeight = 18 * scale + 6;
    int menuWidth = maxTextW + 10 * scale;
    int menuX = WINDOW_WIDTH - menuWidth - 10;
    int menuY = 10;
    int menuHeight = 16;
    int menuItemCount = static_cast<int>(menuItems.size());
    for (int i = 0; i < menuItemCount; ++i)
        menuHeight += itemHeight;

    renderer.drawFilledRect(menuX, menuY, menuWidth, menuHeight, TDT4102::Color{240, 245, 255});
    renderer.drawRect(menuX, menuY, menuWidth, menuHeight, TDT4102::Color::transparent,
                      TDT4102::Color{60, 70, 100});

    for (int i = 0; i < menuItemCount; ++i) {
        int oy = menuY + 8 + i * itemHeight;

        if (i == selected)
            ctx.ui.drawSelectionArrow(menuX + 2 * scale, oy + 4 * scale, scale);

        spriteFont.drawText(renderer, menuItems[static_cast<std::size_t>(i)], menuX + 6 * scale, oy,
                            scale);
    }
}
