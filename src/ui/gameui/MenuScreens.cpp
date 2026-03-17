#include "../../common/StringUtils.h"
#include "../../game_data/Pokedex.h"
#include "../GameUI.h"
#include <algorithm>

void GameUI::drawPartyList(const Player &player, int selected) {
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{140, 155, 190});

    const auto &party = player.getParty();
    int scale = PIXEL_SCALE;
    int slotHeight = 24 * scale;
    int gap = 4 * scale;
    int startY = 12;

    int sy = startY;
    int partyCount = static_cast<int>(party.size());
    for (int i = 0; i < partyCount; ++i) {

        bool isSelected = i == selected;

        TDT4102::Color bgColor =
            isSelected ? TDT4102::Color{200, 215, 255} : TDT4102::Color{230, 235, 250};

        renderer.drawFilledRect(20, sy, WINDOW_WIDTH - 40, slotHeight, bgColor);
        renderer.drawRect(20, sy, WINDOW_WIDTH - 40, slotHeight, TDT4102::Color::transparent,
                          TDT4102::Color::black);

        const Daemon &c = party[static_cast<std::size_t>(i)];

        if (isSelected)
            drawSelectionArrow(28, sy + 6 * scale, scale);

        spriteFont.drawText(renderer, c.getNickname(), 28 + 6 * scale, sy + 2 * scale, scale);

        int lvlX = WINDOW_WIDTH / 2 - 30 * scale;
        spriteFont.drawText(renderer, "Lv" + std::to_string(c.getLevel()), lvlX, sy + 2 * scale,
                            scale);

        int hpBarX = WINDOW_WIDTH / 2 + 10 * scale;
        int hpBarW = 48 * scale;
        drawSpriteHPBar(hpBarX, sy + 4 * scale, hpBarW, c.getCurrentHP(), c.getMaxHP(), scale);

        spriteFont.drawText(renderer,
                            std::to_string(c.getCurrentHP()) + "-" + std::to_string(c.getMaxHP()),
                            hpBarX, sy + 9 * scale, scale);

        std::string typeName = StringUtils::capitalize(elementTypeName(c.getSpecies().primaryType));
        spriteFont.drawText(renderer, typeName, 28 + 6 * scale, sy + 14 * scale, scale - 1);

        sy += slotHeight + gap;
    }
}

void GameUI::drawSummaryScreen(const Daemon &daemon, const Pokedex &pokedex, int page) {
    loadBattleAssets();
    int scale = PIXEL_SCALE;
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{150, 160, 195});

    const std::string &speciesName = daemon.getSpecies().name;
    loadDaemonSprite(speciesName);
    std::string spriteId = "daemon_" + speciesName;
    if (renderer.hasTexture(spriteId)) {
        TDT4102::Image &img = renderer.getTexture(spriteId);
        int sprW = img.width * scale;
        int sprH = img.height * scale;
        int sprX = WINDOW_WIDTH - sprW - 24;
        int sprY = 8;
        renderer.drawSpriteRaw(spriteId, sprX, sprY, sprW, sprH);
    }

    int headerY = 8;
    spriteFont.drawText(renderer, daemon.getNickname(), 24, headerY, scale);
    std::string lvlStr = "Lv" + std::to_string(daemon.getLevel());
    spriteFont.drawText(renderer, lvlStr,
                        24 + spriteFont.getTextWidth(daemon.getNickname(), scale) + 8 * scale,
                        headerY, scale);

    int subY = headerY + 18 * scale;
    if (daemon.getNickname() != daemon.getSpecies().name) {
        spriteFont.drawText(renderer, daemon.getSpecies().name, 24, subY, scale - 1);
    }

    int typeY = subY + 14 * scale;
    std::string primaryType =
        StringUtils::capitalize(elementTypeName(daemon.getSpecies().primaryType));
    std::string secondaryType =
        StringUtils::capitalize(elementTypeName(daemon.getSpecies().secondaryType));

    TDT4102::Color typeBg{90, 100, 145};
    int typeBoxH = 14 * scale;
    int typeTextY = typeY + 2 * scale;

    int tw1 = spriteFont.getTextWidth(primaryType, scale - 1) + 6 * scale;
    renderer.drawFilledRect(24, typeY, tw1, typeBoxH, typeBg);
    spriteFont.drawText(renderer, primaryType, 24 + 3 * scale, typeTextY, scale - 1);

    if (daemon.getSpecies().primaryType != daemon.getSpecies().secondaryType) {
        int tw2 = spriteFont.getTextWidth(secondaryType, scale - 1) + 6 * scale;
        renderer.drawFilledRect(24 + tw1 + 4 * scale, typeY, tw2, typeBoxH, typeBg);
        spriteFont.drawText(renderer, secondaryType, 24 + tw1 + 4 * scale + 3 * scale, typeTextY,
                            scale - 1);
    }

    int divY = typeY + typeBoxH + 4 * scale;
    renderer.drawFilledRect(20, divY, WINDOW_WIDTH / 2, 2, TDT4102::Color{120, 130, 170});

    if (page == 0) {
        int infoY = divY + 4 * scale;
        spriteFont.drawText(renderer, "HP", 24, infoY, scale);
        int hpBarX = 54 + 16 * scale;
        int hpBarW = 48 * scale;
        drawSpriteHPBar(hpBarX, infoY + 4 * scale, hpBarW, daemon.getCurrentHP(), daemon.getMaxHP(),
                        scale);
        spriteFont.drawText(renderer,
                            std::to_string(daemon.getCurrentHP()) + "-" +
                                std::to_string(daemon.getMaxHP()),
                            hpBarX + hpBarW + 4 * scale, infoY, scale);

        int expY = infoY + 14 * scale;
        spriteFont.drawText(renderer, "EXP", 24, expY, scale);
        int expBarX = 54 + 16 * scale;
        int expBarW = 48 * scale;
        int expNeeded = daemon.getExpNeeded();
        drawSpriteEXPBar(expBarX, expY + 4 * scale, expBarW, daemon.getExp(), expNeeded, scale);
        spriteFont.drawText(renderer,
                            std::to_string(daemon.getExp()) + "-" + std::to_string(expNeeded),
                            expBarX + expBarW + 4 * scale, expY, scale);

        int movesHeaderY = expY + 16 * scale;
        renderer.drawFilledRect(20, movesHeaderY, WINDOW_WIDTH - 40, 2,
                                TDT4102::Color{120, 130, 170});
        int moveLabelY = movesHeaderY + 4 * scale;
        spriteFont.drawText(renderer, "MOVES", 24, moveLabelY, scale);

        int moveStartY = moveLabelY + 14 * scale;
        int availableH = (WINDOW_HEIGHT - 16 * scale) - moveStartY;
        int moveSlotH = availableH / 4;

        const auto &moves = daemon.getMoves();
        int my = moveStartY;
        for (int i = 0; i < 4; ++i) {

            if (i % 2 == 0)
                renderer.drawFilledRect(24, my, WINDOW_WIDTH - 48, moveSlotH - 2,
                                        TDT4102::Color{135, 145, 180});

            if (moves[static_cast<std::size_t>(i)].moveId < 0) {
                spriteFont.drawText(renderer, "---", 36, my + 2 * scale, scale);
                continue;
            }

            const MoveData &moveData = pokedex.getMove(moves[static_cast<std::size_t>(i)].moveId);

            spriteFont.drawText(renderer, moveData.name, 36, my + 2 * scale, scale);

            std::string mType = StringUtils::capitalize(elementTypeName(moveData.type));
            spriteFont.drawText(renderer, mType, WINDOW_WIDTH / 2 - 10 * scale, my + 2 * scale,
                                scale - 1);

            std::string pp = "PP " + std::to_string(moves[static_cast<std::size_t>(i)].currentPP) +
                             "-" + std::to_string(moves[static_cast<std::size_t>(i)].maxPP);
            int ppX = WINDOW_WIDTH - 24 - spriteFont.getTextWidth(pp, scale);
            spriteFont.drawText(renderer, pp, ppX, my + 2 * scale, scale);

            my += moveSlotH;
        }
    } else {
        int infoY = divY + 4 * scale;
        spriteFont.drawText(renderer, "STATS", 24, infoY, scale);

        static const std::string statNames[] = {"HP", "Atk", "Def", "SpA", "SpD", "Spe"};
        int statStartY = infoY + 18 * scale;

        for (int i = 0; i < 6; ++i) {
            int sy = statStartY + i * 14 * scale;
            spriteFont.drawText(renderer, statNames[i], 36, sy, scale);

            int statVal = daemon.getStat(i);
            std::string valStr = std::to_string(statVal);
            int valX = 36 + 30 * scale;
            spriteFont.drawText(renderer, valStr, valX, sy, scale);

            int barX = valX + 24 * scale;
            int barMaxW = WINDOW_WIDTH - barX - 40;
            int barW = std::min(barMaxW, statVal * barMaxW / 200);
            int barH = 8 * scale;
            renderer.drawFilledRect(barX, sy + 3 * scale, barMaxW, barH,
                                    TDT4102::Color{120, 125, 155});
            if (barW > 0) {
                TDT4102::Color barColor{100, 180, 255};
                if (statVal > 120)
                    barColor = TDT4102::Color{100, 220, 130};
                else if (statVal < 50)
                    barColor = TDT4102::Color{255, 130, 100};
                renderer.drawFilledRect(barX, sy + 3 * scale, barW, barH, barColor);
            }
        }
    }

    int footerY = WINDOW_HEIGHT - 16 * scale;
    std::string pageText = (page == 0) ? "Info  LR:Stats" : "Stats  LR:Info";
    int pageTextX = (WINDOW_WIDTH - spriteFont.getTextWidth(pageText, scale - 1)) / 2;
    spriteFont.drawText(renderer, pageText, pageTextX, footerY, scale - 1);

    spriteFont.drawText(renderer, "B:Back", 24, footerY, scale - 1);
}

void GameUI::drawBagScreen(const Player &player, const Pokedex &pokedex, int selected) {
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{170, 155, 130});

    int scale = PIXEL_SCALE;
    spriteFont.drawText(renderer, "BAG", 28, 10, scale);

    const auto &inventory = player.getInventory();

    if (inventory.empty()) {
        spriteFont.drawText(renderer, "No items", 28, 60, scale);
        return;
    }

    int slotHeight = 16 * scale + 8;
    int startY = 10 + 20 * scale;

    int sy = startY;
    int inventoryCount = static_cast<int>(inventory.size());
    for (int i = 0; i < inventoryCount; ++i) {

        bool isSelected = i == selected;

        TDT4102::Color bgColor =
            isSelected ? TDT4102::Color{255, 230, 180} : TDT4102::Color{240, 220, 200};

        renderer.drawFilledRect(20, sy, WINDOW_WIDTH - 40, slotHeight, bgColor);
        renderer.drawRect(20, sy, WINDOW_WIDTH - 40, slotHeight, TDT4102::Color::transparent,
                          TDT4102::Color::black);

        const ItemData &item = pokedex.getItem(inventory[static_cast<std::size_t>(i)].itemId);

        if (isSelected)
            drawSelectionArrow(28, sy + 4 * scale, scale);

        spriteFont.drawText(renderer, item.name, 28 + 6 * scale, sy + 4, scale);

        std::string qtyStr = "x" + std::to_string(inventory[static_cast<std::size_t>(i)].quantity);
        int qtyX = WINDOW_WIDTH - 60 - spriteFont.getTextWidth(qtyStr, scale);
        spriteFont.drawText(renderer, qtyStr, qtyX, sy + 4, scale);

        sy += slotHeight + 8;
    }

    if (selected >= 0 && selected < static_cast<int>(inventory.size())) {
        const ItemData &item =
            pokedex.getItem(inventory[static_cast<std::size_t>(selected)].itemId);
        int descY = WINDOW_HEIGHT - 30 * scale;
        drawNarrowTextBar(20, descY, 180, scale);
        spriteFont.drawText(renderer, item.description, 28 + 4 * scale, descY + 5 * scale, scale);
    }
}
