#include "../../common/StringUtils.h"
#include "../../game_data/Pokedex.h"
#include "../GameUI.h"
#include <algorithm>

namespace {

std::string moveCategoryName(MoveCategory category) {
    switch (category) {
    case MoveCategory::physical:
        return "Physical";
    case MoveCategory::special:
        return "Special";
    case MoveCategory::status:
        return "Status";
    }
    return "Unknown";
}

std::string moveTypeName(ElementType type) {
    return StringUtils::capitalize(elementTypeName(type));
}

std::string moveMetricText(const std::string &label, int value, bool dashWhenEmpty = false) {
    if (dashWhenEmpty && value <= 0)
        return label + " --";
    return label + " " + std::to_string(value);
}

int drawSummaryHeader(GameUI &ui, const Daemon &daemon) {
    Renderer &renderer = ui.getRenderer();
    SpriteFont &spriteFont = ui.getSpriteFont();
    const int scale = PIXEL_SCALE;

    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{150, 160, 195});

    const std::string &speciesName = daemon.getSpecies().name;
    ui.loadDaemonSprite(speciesName);
    const std::string spriteId = "daemon_" + speciesName;
    if (renderer.hasTexture(spriteId)) {
        TDT4102::Image &img = renderer.getTexture(spriteId);
        const int sprW = img.width * scale;
        const int sprH = img.height * scale;
        const int sprX = WINDOW_WIDTH - sprW - 24;
        const int sprY = 8;
        renderer.drawSpriteRaw(spriteId, sprX, sprY, sprW, sprH);
    }

    const int headerY = 8;
    spriteFont.drawText(renderer, daemon.getNickname(), 24, headerY, scale);
    const std::string levelText = "Lv" + std::to_string(daemon.getLevel());
    spriteFont.drawText(renderer, levelText,
                        24 + spriteFont.getTextWidth(daemon.getNickname(), scale) + 8 * scale,
                        headerY, scale);

    const int subY = headerY + 18 * scale;
    if (daemon.getNickname() != daemon.getSpecies().name)
        spriteFont.drawText(renderer, daemon.getSpecies().name, 24, subY, scale - 1);

    const int typeY = subY + 14 * scale;
    const std::string primaryType = moveTypeName(daemon.getSpecies().primaryType);
    const std::string secondaryType = moveTypeName(daemon.getSpecies().secondaryType);

    const TDT4102::Color typeBg{90, 100, 145};
    const int typeBoxH = 14 * scale;
    const int typeTextY = typeY + 2 * scale;

    const int primaryWidth = spriteFont.getTextWidth(primaryType, scale - 1) + 6 * scale;
    renderer.drawFilledRect(24, typeY, primaryWidth, typeBoxH, typeBg);
    spriteFont.drawText(renderer, primaryType, 24 + 3 * scale, typeTextY, scale - 1);

    if (daemon.getSpecies().primaryType != daemon.getSpecies().secondaryType) {
        const int secondaryWidth = spriteFont.getTextWidth(secondaryType, scale - 1) + 6 * scale;
        renderer.drawFilledRect(24 + primaryWidth + 4 * scale, typeY, secondaryWidth, typeBoxH,
                                typeBg);
        spriteFont.drawText(renderer, secondaryType, 24 + primaryWidth + 7 * scale, typeTextY,
                            scale - 1);
    }

    const int dividerY = typeY + typeBoxH + 4 * scale;
    renderer.drawFilledRect(20, dividerY, WINDOW_WIDTH / 2, 2, TDT4102::Color{120, 130, 170});
    return dividerY;
}

void drawMoveDetailBox(GameUI &ui, const MoveData *moveData, const MoveSlot *moveSlot,
                       const std::string &title, int x, int y, int width, int height,
                       bool showName = true) {
    Renderer &renderer = ui.getRenderer();
    SpriteFont &spriteFont = ui.getSpriteFont();
    const int scale = PIXEL_SCALE;
    const int nameScale = scale - 1;
    const int infoScale = scale - 1;
    const int valueGap = 3 * scale;
    const int leftColumnX = x + 6 * scale;
    const int rightColumnX = x + width / 2;
    const int contentWidth = width - 12 * scale;
    const int lineStep = 12 * scale;

    renderer.drawFilledRect(x, y, width, height, TDT4102::Color{228, 233, 248});
    renderer.drawRect(x, y, width, height, TDT4102::Color::transparent,
                      TDT4102::Color{70, 80, 110});
    spriteFont.drawText(renderer, title, x + 4 * scale, y + 4 * scale, scale - 1);

    if (moveData == nullptr) {
        spriteFont.drawText(renderer, "No move", leftColumnX, y + 18 * scale, nameScale);
        return;
    }

    int lineY = y + 18 * scale;
    if (showName) {
        spriteFont.drawTextPartial(renderer, moveData->name, moveData->name.size(), leftColumnX,
                                   lineY, nameScale, 1, contentWidth);
        lineY += lineStep;
    }

    const std::string typeText = moveTypeName(moveData->type);
    const std::string categoryText = moveCategoryName(moveData->category);
    const int typeValueX = leftColumnX + spriteFont.getTextWidth("Type", infoScale) + valueGap;
    const int kindValueX = rightColumnX + spriteFont.getTextWidth("Kind", infoScale) + valueGap;
    spriteFont.drawText(renderer, "Type", leftColumnX, lineY, infoScale);
    spriteFont.drawText(renderer, typeText, typeValueX, lineY, infoScale);
    spriteFont.drawText(renderer, "Kind", rightColumnX, lineY, infoScale);
    spriteFont.drawText(renderer, categoryText, kindValueX, lineY, infoScale);

    const std::string powerText = moveMetricText("Power", moveData->power, true);
    const std::string accuracyText = moveMetricText("Acc", moveData->accuracy, true);
    const int ppValue = moveSlot != nullptr ? moveSlot->maxPP : moveData->maxPP;
    const std::string ppText = moveMetricText("PP", ppValue);
    lineY += lineStep;
    spriteFont.drawText(renderer, powerText, leftColumnX, lineY, infoScale);
    spriteFont.drawText(renderer, accuracyText, rightColumnX, lineY, infoScale);
    lineY += lineStep;
    spriteFont.drawText(renderer, ppText, leftColumnX, lineY, infoScale);

    const int descriptionY = lineY + lineStep;
    spriteFont.drawTextPartial(renderer, moveData->description, moveData->description.size(),
                               leftColumnX, descriptionY, infoScale, 1, contentWidth);
}

void drawMoveListPanel(GameUI &ui, const Daemon &daemon, const Pokedex &pokedex, int selectedMove,
                       int x, int y, int width, int height, bool includeKeepOption) {
    Renderer &renderer = ui.getRenderer();
    SpriteFont &spriteFont = ui.getSpriteFont();
    const int scale = PIXEL_SCALE;

    renderer.drawFilledRect(x, y, width, height, TDT4102::Color{220, 226, 241});
    renderer.drawRect(x, y, width, height, TDT4102::Color::transparent,
                      TDT4102::Color{70, 80, 110});
    spriteFont.drawText(renderer, "MOVES", x + 4 * scale, y + 4 * scale, scale - 1);

    const int optionCount = includeKeepOption ? 5 : 4;
    const int rowStartY = y + 14 * scale;
    const int rowHeight = (height - 16 * scale) / optionCount;
    const auto &moves = daemon.getMoves();
    const int textWidth = width - 10 * scale;
    const int nameScale = scale - 1;

    for (int i = 0; i < optionCount; ++i) {
        const int rowY = rowStartY + i * rowHeight;
        if (i == selectedMove) {
            renderer.drawFilledRect(x + scale, rowY - scale / 2, width - 2 * scale,
                                    rowHeight - scale, TDT4102::Color{255, 242, 196});
            ui.drawSelectionArrow(x + scale, rowY + 4 * scale, scale);
        }

        if (includeKeepOption && i == 4) {
            spriteFont.drawTextPartial(renderer, "Keep old moves", 15, x + 6 * scale, rowY,
                                       nameScale, 1, textWidth);
            continue;
        }

        const MoveSlot &moveSlot = moves[static_cast<std::size_t>(i)];
        if (moveSlot.moveId < 0) {
            spriteFont.drawText(renderer, "---", x + 6 * scale, rowY, nameScale);
            continue;
        }

        const MoveData &moveData = pokedex.getMove(moveSlot.moveId);
        spriteFont.drawTextPartial(renderer, moveData.name, moveData.name.size(), x + 6 * scale,
                                   rowY, nameScale, 1, textWidth);
    }
}

} // namespace

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

void GameUI::drawSummaryScreen(const Daemon &daemon, const Pokedex &pokedex, int page,
                               int selectedMove) {
    loadBattleAssets();
    const int scale = PIXEL_SCALE;
    const int divY = drawSummaryHeader(*this, daemon);

    if (page == 0) {
        const int infoY = divY + 4 * scale;
        spriteFont.drawText(renderer, "HP", 24, infoY, scale);
        const int hpBarX = 54 + 16 * scale;
        const int hpBarW = 48 * scale;
        drawSpriteHPBar(hpBarX, infoY + 4 * scale, hpBarW, daemon.getCurrentHP(), daemon.getMaxHP(),
                        scale);
        spriteFont.drawText(renderer,
                            std::to_string(daemon.getCurrentHP()) + "-" +
                                std::to_string(daemon.getMaxHP()),
                            hpBarX + hpBarW + 4 * scale, infoY, scale);

        const int expY = infoY + 14 * scale;
        spriteFont.drawText(renderer, "EXP", 24, expY, scale);
        const int expBarX = 54 + 16 * scale;
        const int expBarW = 48 * scale;
        const int expNeeded = daemon.getExpNeeded();
        drawSpriteEXPBar(expBarX, expY + 4 * scale, expBarW, daemon.getExpProgress(), expNeeded,
                         scale);
        spriteFont.drawText(
            renderer, std::to_string(daemon.getExpProgress()) + "-" + std::to_string(expNeeded),
            expBarX + expBarW + 4 * scale, expY, scale);

        const int movesHeaderY = expY + 16 * scale;
        renderer.drawFilledRect(20, movesHeaderY, WINDOW_WIDTH - 40, 2,
                                TDT4102::Color{120, 130, 170});
        const int moveLabelY = movesHeaderY + 4 * scale;
        spriteFont.drawText(renderer, "MOVES", 24, moveLabelY, scale);

        const int moveStartY = moveLabelY + 14 * scale;
        const int availableH = (WINDOW_HEIGHT - 16 * scale) - moveStartY;
        const int moveSlotH = availableH / 4;

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

            const std::string mType = moveTypeName(moveData.type);
            spriteFont.drawText(renderer, mType, WINDOW_WIDTH / 2 - 10 * scale, my + 2 * scale,
                                scale - 1);

            std::string pp = "PP " + std::to_string(moves[static_cast<std::size_t>(i)].currentPP) +
                             "-" + std::to_string(moves[static_cast<std::size_t>(i)].maxPP);
            int ppX = WINDOW_WIDTH - 24 - spriteFont.getTextWidth(pp, scale);
            spriteFont.drawText(renderer, pp, ppX, my + 2 * scale, scale);

            my += moveSlotH;
        }
    } else if (page == 1) {
        const int infoY = divY + 4 * scale;
        spriteFont.drawText(renderer, "STATS", 24, infoY, scale);

        static const std::string statNames[] = {"HP", "Atk", "Def", "SpA", "SpD", "Spe"};
        const int statStartY = infoY + 18 * scale;

        for (int i = 0; i < 6; ++i) {
            const int sy = statStartY + i * 14 * scale;
            spriteFont.drawText(renderer, statNames[i], 36, sy, scale);

            const int statVal = daemon.getStat(i);
            const std::string valStr = std::to_string(statVal);
            const int valX = 36 + 30 * scale;
            spriteFont.drawText(renderer, valStr, valX, sy, scale);

            const int barX = valX + 24 * scale;
            const int barMaxW = WINDOW_WIDTH - barX - 40;
            const int barW = std::min(barMaxW, statVal * barMaxW / 200);
            const int barH = 8 * scale;
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
    } else {
        const int listX = 24;
        const int listY = divY + 6 * scale;
        const int listW = 84 * scale;
        const int listH = WINDOW_HEIGHT - listY - 22 * scale;
        const int detailX = listX + listW + 4 * scale;
        const int detailY = listY;
        const int detailW = WINDOW_WIDTH - detailX - 24;
        const int detailH = listH;

        drawMoveListPanel(*this, daemon, pokedex, std::clamp(selectedMove, 0, 3), listX, listY,
                          listW, listH, false);

        const int clampedMove = std::clamp(selectedMove, 0, 3);
        const MoveSlot &moveSlot = daemon.getMoves()[static_cast<std::size_t>(clampedMove)];
        const MoveData *moveData = nullptr;
        if (moveSlot.moveId >= 0)
            moveData = &pokedex.getMove(moveSlot.moveId);
        drawMoveDetailBox(*this, moveData, moveSlot.moveId >= 0 ? &moveSlot : nullptr,
                          "MOVE DETAILS", detailX, detailY, detailW, detailH, false);
    }

    const int footerY = WINDOW_HEIGHT - 16 * scale;
    std::string pageText;
    if (page == 0)
        pageText = "Info  LR:Page";
    else if (page == 1)
        pageText = "Stats  LR:Page";
    else
        pageText = "Moves  UD:Select  LR:Page";
    const int pageTextX = (WINDOW_WIDTH - spriteFont.getTextWidth(pageText, scale - 1)) / 2;
    spriteFont.drawText(renderer, pageText, pageTextX, footerY, scale - 1);

    spriteFont.drawText(renderer, "B:Back", 24, footerY, scale - 1);
}

void GameUI::drawMoveLearningScreen(const Daemon &daemon, const Pokedex &pokedex, int selectedMove,
                                    int newMoveId) {
    const int scale = PIXEL_SCALE;
    const int clampedSelection = std::clamp(selectedMove, 0, 4);
    const MoveData &newMove = pokedex.getMove(newMoveId);

    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{162, 154, 188});

    spriteFont.drawText(renderer, daemon.getNickname(), 24, 8, scale);
    spriteFont.drawText(renderer, "Choose a move to forget.", 24, 26 * scale, scale - 1);

    const int listX = 24;
    const int listY = 40 * scale;
    const int listW = 84 * scale;
    const int listH = WINDOW_HEIGHT - listY - 18 * scale;
    const int detailX = listX + listW + 4 * scale;
    const int detailW = WINDOW_WIDTH - detailX - 24;
    const int currentH = (listH - 3 * scale) / 2;
    const int newY = listY + currentH + 3 * scale;

    drawMoveListPanel(*this, daemon, pokedex, clampedSelection, listX, listY, listW, listH, true);

    const MoveData *currentMove = nullptr;
    const MoveSlot *currentSlot = nullptr;
    if (clampedSelection < 4) {
        const MoveSlot &slot = daemon.getMoves()[static_cast<std::size_t>(clampedSelection)];
        if (slot.moveId >= 0) {
            currentMove = &pokedex.getMove(slot.moveId);
            currentSlot = &slot;
        }
    }

    if (clampedSelection >= 4) {
        renderer.drawFilledRect(detailX, listY, detailW, currentH, TDT4102::Color{228, 233, 248});
        renderer.drawRect(detailX, listY, detailW, currentH, TDT4102::Color::transparent,
                          TDT4102::Color{70, 80, 110});
        spriteFont.drawText(renderer, "FORGET", detailX + 4 * scale, listY + 4 * scale, scale - 1);
        spriteFont.drawText(renderer, "Keep current moves.", detailX + 6 * scale,
                            listY + 18 * scale, scale);
    } else {
        drawMoveDetailBox(*this, currentMove, currentSlot, "FORGET", detailX, listY, detailW,
                          currentH, false);
    }
    drawMoveDetailBox(*this, &newMove, nullptr, "LEARN", detailX, newY, detailW, currentH);

    const int footerY = WINDOW_HEIGHT - 16 * scale;
    spriteFont.drawText(renderer, "A:Choose  B:Keep old moves", 24, footerY, scale - 1);
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

void GameUI::drawShopScreen(const Player &player, const Pokedex &pokedex,
                            const std::vector<int> &itemIds, int selected, const std::string &title,
                            const std::string &footerText) {
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{182, 172, 145});

    const int scale = PIXEL_SCALE;
    const int outerPadding = 5 * scale;
    const int headerHeight = 24 * scale;
    const int sectionGap = 3 * scale;
    const int listWidth = 132 * scale;
    const int listHeight = 110 * scale;
    const int detailGap = 3 * scale;
    const int panelY = headerHeight + 10 * scale;

    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, headerHeight, TDT4102::Color{98, 82, 56});
    spriteFont.drawText(renderer, title, outerPadding, 4 * scale, scale);

    const std::string moneyText = "$" + std::to_string(player.getMoney());
    const int moneyTextWidth = spriteFont.getTextWidth(moneyText, scale);
    spriteFont.drawText(renderer, moneyText, WINDOW_WIDTH - moneyTextWidth - outerPadding,
                        4 * scale, scale);

    const int listX = outerPadding;
    const int listY = panelY;
    renderer.drawFilledRect(listX, listY, listWidth, listHeight, TDT4102::Color{238, 231, 214});
    renderer.drawRect(listX, listY, listWidth, listHeight, TDT4102::Color::transparent,
                      TDT4102::Color{70, 60, 40});

    if (itemIds.empty()) {
        spriteFont.drawText(renderer, "Sold out", listX + 8 * scale, listY + 8 * scale, scale);
    } else {
        int sy = listY + 6 * scale;
        for (std::size_t i = 0; i < itemIds.size(); ++i) {
            const bool isSelected = static_cast<int>(i) == selected;
            const ItemData &item = pokedex.getItem(itemIds[i]);
            if (isSelected) {
                renderer.drawFilledRect(listX + scale, sy - scale / 2, listWidth - 2 * scale,
                                        16 * scale, TDT4102::Color{255, 239, 181});
                drawSelectionArrow(listX + scale / 2, sy + 4 * scale, scale);
            }

            spriteFont.drawText(renderer, item.name, listX + 6 * scale, sy, scale);
            const std::string priceText = "$" + std::to_string(item.value);
            const int priceX =
                listX + listWidth - spriteFont.getTextWidth(priceText, scale) - 8 * scale;
            spriteFont.drawText(renderer, priceText, priceX, sy, scale);
            sy += 16 * scale;
        }
    }

    const int detailX = listX + listWidth + detailGap;
    const int detailY = listY;
    const int detailW = WINDOW_WIDTH - detailX - outerPadding;
    const int detailH = listHeight;
    renderer.drawFilledRect(detailX, detailY, detailW, detailH, TDT4102::Color{248, 244, 232});
    renderer.drawRect(detailX, detailY, detailW, detailH, TDT4102::Color::transparent,
                      TDT4102::Color{70, 60, 40});

    if (!itemIds.empty()) {
        const ItemData &item = pokedex.getItem(itemIds[static_cast<std::size_t>(selected)]);
        const int detailTextX = detailX + 8 * scale;
        spriteFont.drawText(renderer, item.name, detailTextX, detailY + 6 * scale, scale);

        const std::string ownedText = "Owned: " + std::to_string(player.getItemCount(item.id));
        spriteFont.drawText(renderer, ownedText, detailTextX, detailY + 22 * scale, scale);

        const std::string costText = "Price: $" + std::to_string(item.value);
        spriteFont.drawText(renderer, costText, detailTextX, detailY + 38 * scale, scale);

        const int descriptionX = detailTextX;
        const int descriptionY = detailY + 54 * scale;
        const int descriptionWidth = detailW - 16 * scale;
        spriteFont.drawTextPartial(renderer, item.description, item.description.size(),
                                   descriptionX, descriptionY, scale, 1, descriptionWidth);
    }

    const int hintY = WINDOW_HEIGHT - 20 * scale;
    drawNarrowTextBar(outerPadding, hintY, 220, scale);
    spriteFont.drawText(renderer, footerText, outerPadding + sectionGap * 4, hintY + 5 * scale,
                        scale);
}

void GameUI::drawPlayerStatsScreen(const Player &player) {
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{210, 220, 238});

    const int scale = PIXEL_SCALE;
    const int outerPadding = 8 * scale;
    const int gap = 4 * scale;
    const int headerHeight = 18 * scale;
    const int leftPanelWidth = 88 * scale;
    const int panelY = outerPadding + headerHeight + gap;
    const int panelHeight = WINDOW_HEIGHT - panelY - 22 * scale;
    const int rightPanelX = outerPadding + leftPanelWidth + gap;
    const int rightPanelWidth = WINDOW_WIDTH - rightPanelX - outerPadding;

    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, headerHeight + outerPadding,
                            TDT4102::Color{68, 86, 128});
    spriteFont.drawText(renderer, "TRAINER STATS", outerPadding, 4 * scale, scale);

    renderer.drawFilledRect(outerPadding, panelY, leftPanelWidth, panelHeight,
                            TDT4102::Color{236, 241, 252});
    renderer.drawRect(outerPadding, panelY, leftPanelWidth, panelHeight,
                      TDT4102::Color::transparent, TDT4102::Color{72, 82, 108});

    renderer.drawFilledRect(rightPanelX, panelY, rightPanelWidth, panelHeight,
                            TDT4102::Color{244, 247, 255});
    renderer.drawRect(rightPanelX, panelY, rightPanelWidth, panelHeight,
                      TDT4102::Color::transparent, TDT4102::Color{72, 82, 108});

    int lineY = panelY + 6 * scale;
    spriteFont.drawText(renderer, "Name", outerPadding + 4 * scale, lineY, scale - 1);
    lineY += 12 * scale;
    spriteFont.drawTextPartial(renderer, player.getName(), player.getName().size(),
                               outerPadding + 4 * scale, lineY, scale - 1, 1,
                               leftPanelWidth - 8 * scale);

    lineY += 14 * scale;
    spriteFont.drawText(renderer, "Money", outerPadding + 4 * scale, lineY, scale - 1);
    lineY += 12 * scale;
    const std::string moneyText = "$" + std::to_string(player.getMoney());
    spriteFont.drawText(renderer, moneyText, outerPadding + 4 * scale, lineY, scale - 1);

    lineY += 14 * scale;
    spriteFont.drawText(renderer, "Badges", outerPadding + 4 * scale, lineY, scale - 1);
    lineY += 12 * scale;
    spriteFont.drawText(renderer, std::to_string(player.badgeCount()), outerPadding + 4 * scale,
                        lineY, scale - 1);

    lineY += 14 * scale;
    spriteFont.drawText(renderer, "Party", outerPadding + 4 * scale, lineY, scale - 1);
    lineY += 12 * scale;
    spriteFont.drawText(renderer, std::to_string(player.partySize()), outerPadding + 4 * scale,
                        lineY, scale - 1);

    lineY += 14 * scale;
    spriteFont.drawText(renderer, "Dex", outerPadding + 4 * scale, lineY, scale - 1);
    lineY += 12 * scale;
    const std::string dexText =
        std::to_string(player.seenCount()) + "/" + std::to_string(player.caughtCount());
    spriteFont.drawText(renderer, dexText, outerPadding + 4 * scale, lineY, scale - 1);

    int contentX = rightPanelX + 6 * scale;
    int contentY = panelY + 6 * scale;
    spriteFont.drawText(renderer, "BADGES", contentX, contentY, scale);

    const auto &badges = player.getBadges();
    if (badges.empty()) {
        spriteFont.drawText(renderer, "No badges yet.", contentX, contentY + 16 * scale, scale - 1);
    } else {
        const int badgeRowHeight = 16 * scale;
        const int badgeWidth = rightPanelWidth - 12 * scale;
        for (std::size_t i = 0; i < badges.size(); ++i) {
            const int rowY = contentY + 14 * scale + static_cast<int>(i) * badgeRowHeight;
            renderer.drawFilledRect(rightPanelX + 3 * scale, rowY - scale / 2, badgeWidth,
                                    14 * scale, TDT4102::Color{224, 230, 246});
            spriteFont.drawText(renderer, badges[i], contentX, rowY, scale - 1);
        }
    }

    const int hintY = WINDOW_HEIGHT - 16 * scale;
    drawNarrowTextBar(outerPadding, hintY, 150, scale);
    spriteFont.drawText(renderer, "Z/X: Back", outerPadding + 4 * scale, hintY + 4 * scale, scale);
}
