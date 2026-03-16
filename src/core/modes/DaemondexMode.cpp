#include "DaemondexMode.h"
#include "../../data/Pokedex.h"
#include "../../state/player/Player.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"
#include "../../ui/Renderer.h"
#include "../../ui/SpriteFont.h"
#include "../StringUtils.h"

void DaemondexMode::update(GameContext &ctx, InputManager &input) {
    int totalEntries = ctx.pokedex.speciesCount();
    if (totalEntries <= 1)
        return;
    totalEntries -= 1; // exclude dummy at index 0

    switch (subState) {
    case SubState::list: {
        ctx.ui.navigateVertical(selected, totalEntries);

        // Keep selected within visible window
        if (selected < scrollOffset)
            scrollOffset = selected;
        if (selected >= scrollOffset + VISIBLE_ROWS)
            scrollOffset = selected - VISIBLE_ROWS + 1;

        if (input.isCancelPressed()) {
            ctx.pushRequest(ModeRequest::changeState(GameState::menu));
            return;
        }

        if (input.isConfirmPressed()) {
            int speciesId = selected + 1;
            const Player &player = ctx.world.getPlayer();
            if (player.hasSeen(speciesId)) {
                ctx.playSound(SoundEffect::select);
                subState = SubState::detail;
            }
        }
        break;
    }
    case SubState::detail: {
        if (input.isCancelPressed() || input.isConfirmPressed()) {
            subState = SubState::list;
        }
        break;
    }
    }
}

void DaemondexMode::render(GameContext &ctx) {
    switch (subState) {
    case SubState::list:
        drawList(ctx);
        break;
    case SubState::detail:
        drawDetail(ctx);
        break;
    }
}

void DaemondexMode::drawList(GameContext &ctx) {
    Renderer &renderer = ctx.ui.getRenderer();
    SpriteFont &font = ctx.ui.getSpriteFont();
    const Player &player = ctx.world.getPlayer();
    int totalEntries = ctx.pokedex.speciesCount();
    if (totalEntries == 0)
        return;
    totalEntries -= 1;
    int scale = PIXEL_SCALE;

    // Background
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{230, 235, 245});

    // Title bar
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, 14 * scale, TDT4102::Color{70, 90, 140});
    font.drawText(renderer, "DAEMONDEX", 8 * scale, 1 * scale, scale);

    // Seen/caught counts
    std::string countsText = "Seen: " + std::to_string(player.seenCount()) +
                             "  Caught: " + std::to_string(player.caughtCount());
    int countsW = font.getTextWidth(countsText, scale);
    font.drawText(renderer, countsText, WINDOW_WIDTH - countsW - 8 * scale, 1 * scale, scale);

    // List area
    int listY = 16 * scale;
    int rowHeight = 18 * scale;
    int listX = 8 * scale;

    for (int i = 0; i < VISIBLE_ROWS && (scrollOffset + i) < totalEntries; ++i) {
        int entryIndex = scrollOffset + i;
        int speciesId = entryIndex + 1;
        int rowY = listY + i * rowHeight;

        bool isSelected = (entryIndex == selected);
        bool seen = player.hasSeen(speciesId);
        bool caught = player.hasCaught(speciesId);

        // Highlight selected row
        if (isSelected) {
            renderer.drawFilledRect(listX - 2 * scale, rowY, WINDOW_WIDTH - 12 * scale,
                                    rowHeight - 2, TDT4102::Color{180, 200, 240});
        }

        // Dex number
        std::string numStr = std::to_string(speciesId);
        while (numStr.size() < 3)
            numStr = "0" + numStr;
        font.drawText(renderer, numStr, listX + 2 * scale, rowY + 2 * scale, scale);

        // Name or ???
        int nameX = listX + 25 * scale;
        if (seen) {
            const Species &sp = ctx.pokedex.getSpecies(speciesId);
            font.drawText(renderer, sp.name, nameX, rowY + 2 * scale, scale);
        } else {
            font.drawText(renderer, "???", nameX, rowY + 2 * scale, scale);
        }

        // Status icon: pokeball for caught, eye for seen
        int iconX = WINDOW_WIDTH - 50 * scale;
        if (caught) {
            font.drawText(renderer, "CAUGHT", iconX, rowY + 2 * scale, scale);
        } else if (seen) {
            font.drawText(renderer, "SEEN", iconX, rowY + 2 * scale, scale);
        }

        // Selection arrow
        if (isSelected) {
            ctx.ui.drawSelectionArrow(listX - 2 * scale, rowY + 4 * scale, scale);
        }
    }

    // Scrollbar
    if (totalEntries > VISIBLE_ROWS) {
        int barX = WINDOW_WIDTH - 4 * scale;
        int barTop = listY;
        int barFullH = static_cast<int>(VISIBLE_ROWS) * rowHeight;
        int thumbH =
            std::max(8, barFullH * static_cast<int>(VISIBLE_ROWS) / static_cast<int>(totalEntries));
        int thumbY = barTop + (barFullH - thumbH) * static_cast<int>(scrollOffset) /
                                  static_cast<int>(totalEntries - VISIBLE_ROWS);

        renderer.drawFilledRect(barX, barTop, 2 * scale, barFullH, TDT4102::Color{200, 200, 210});
        renderer.drawFilledRect(barX, thumbY, 2 * scale, thumbH, TDT4102::Color{100, 110, 140});
    }
}

void DaemondexMode::drawDetail(GameContext &ctx) {
    Renderer &renderer = ctx.ui.getRenderer();
    SpriteFont &font = ctx.ui.getSpriteFont();
    const Player &player = ctx.world.getPlayer();
    int scale = PIXEL_SCALE;

    int speciesId = selected + 1;
    const Species &sp = ctx.pokedex.getSpecies(speciesId);

    // Background
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{240, 245, 255});

    // Border frame
    int margin = 6 * scale;
    renderer.drawRect(margin, margin, WINDOW_WIDTH - 2 * margin, WINDOW_HEIGHT - 2 * margin,
                      TDT4102::Color::transparent, TDT4102::Color{70, 90, 140});

    // Dex number + name
    std::string numStr = std::to_string(speciesId);
    while (numStr.size() < 3)
        numStr = "0" + numStr;
    std::string header = "No." + numStr + "  " + sp.name;
    font.drawText(renderer, header, 12 * scale, 10 * scale, scale);

    // Sprite (if caught, show front sprite; if only seen, show silhouette)
    int spriteX = 14 * scale;
    int spriteY = 24 * scale;
    int spriteSize = 64 * scale;

    std::string frontId = "daemon_" + sp.name;
    if (!renderer.hasTexture(frontId))
        ctx.ui.loadDaemonSprite(sp.name);

    if (renderer.hasTexture(frontId)) {
        renderer.drawSpriteRaw(frontId, spriteX, spriteY, spriteSize, spriteSize);
    }

    int infoX = spriteX;
    int infoY = spriteY + spriteSize + 10 * scale;

    // Types
    std::string type1 = StringUtils::capitalize(elementTypeName(sp.primaryType));
    font.drawText(renderer, "Type: " + type1, infoX, infoY, scale);

    if (sp.secondaryType != sp.primaryType) {
        std::string type2 = StringUtils::capitalize(elementTypeName(sp.secondaryType));
        font.drawText(renderer, " / " + type2, infoX + font.getTextWidth("Type: " + type1, scale),
                      infoY, scale);
    }

    if (player.hasCaught(speciesId)) {
        // Base stats
        int statsY = infoY + 16 * scale;
        font.drawText(renderer, "HP:  " + std::to_string(sp.baseStats.hp), infoX, statsY, scale);
        font.drawText(renderer, "ATK: " + std::to_string(sp.baseStats.attack), infoX,
                      statsY + 14 * scale, scale);
        font.drawText(renderer, "DEF: " + std::to_string(sp.baseStats.defense), infoX,
                      statsY + 28 * scale, scale);

        int col2X = infoX + 60 * scale;
        font.drawText(renderer, "SPA: " + std::to_string(sp.baseStats.specialAttack), col2X, statsY,
                      scale);
        font.drawText(renderer, "SPD: " + std::to_string(sp.baseStats.specialDefense), col2X,
                      statsY + 14 * scale, scale);
        font.drawText(renderer, "SPE: " + std::to_string(sp.baseStats.speed), col2X,
                      statsY + 28 * scale, scale);
    } else {
        int statsY = infoY + 16 * scale;
        font.drawText(renderer, "Catch this Daemon to", infoX, statsY, scale);
        font.drawText(renderer, "see more details!", infoX, statsY + 14 * scale, scale);
    }

    // Status
    int statusY = WINDOW_HEIGHT - 20 * scale;
    if (player.hasCaught(speciesId))
        font.drawText(renderer, "Status: CAUGHT", 12 * scale, statusY, scale);
    else
        font.drawText(renderer, "Status: SEEN", 12 * scale, statusY, scale);
}
