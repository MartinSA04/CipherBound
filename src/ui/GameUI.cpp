#include "GameUI.h"
#include "../core/StringUtils.h"
#include "../data/Pokedex.h"
#include <algorithm>

GameUI::GameUI()
    : renderer(), input(renderer.getWindow()), overworldRenderer(renderer),
      spriteFont(), currentScreen(ScreenType::title) {
    // Load font and text bar textures early so they're available everywhere
    spriteFont.loadTextures(renderer);
    renderer.loadTexture("ui_text_bar", "assets/sprites/ui/text_bar.png");
}

Renderer &GameUI::getRenderer() { return renderer; }
InputManager &GameUI::getInput() { return input; }
OverworldRenderer &GameUI::getOverworldRenderer() { return overworldRenderer; }
SpriteFont &GameUI::getSpriteFont() { return spriteFont; }

void GameUI::setScreen(ScreenType screen) { currentScreen = screen; }
ScreenType GameUI::getCurrentScreen() const { return currentScreen; }

void GameUI::beginFrame() { renderer.beginFrame(); }
void GameUI::endFrame() { renderer.endFrame(); }
bool GameUI::shouldClose() const { return renderer.shouldClose(); }
void GameUI::updateInput() { input.update(); }

void GameUI::drawOverworld(const Map &map, const Player &player,
                           const std::vector<std::shared_ptr<NPC>> &npcs) {
    overworldRenderer.render(map, player, npcs);
}

void GameUI::loadBattleAssets() {
    if (battleAssetsLoaded)
        return;

    renderer.loadTexture("ui_player_info",
                         "assets/sprites/ui/player_creature_info.png");
    renderer.loadTexture("ui_opponent_info",
                         "assets/sprites/ui/opponent_creature_info.png");
    renderer.loadTexture("ui_hp_bar", "assets/sprites/ui/hp_bar.png");
    renderer.loadTexture("ui_exp_bar", "assets/sprites/ui/exp_bar.png");
    renderer.loadTexture("ui_player_base",
                         "assets/sprites/ui/player_battle_base.png");
    renderer.loadTexture("ui_opponent_base",
                         "assets/sprites/ui/opponent_battle_base.png");
    renderer.loadTexture("player_back",
                         "assets/sprites/player/player_back.png");
    renderer.loadTexture("daemon_ball", "assets/sprites/items/daemon_ball.png");

    battleAssetsLoaded = true;
}

void GameUI::loadDaemonSprite(const std::string &speciesName) {
    std::string frontId = "daemon_" + speciesName;
    std::string backId = "daemon_" + speciesName + "_back";
    std::string frontPath = "assets/sprites/daemons/" + speciesName + ".png";
    std::string backPath =
        "assets/sprites/daemons/" + speciesName + "_back.png";

    if (!renderer.hasTexture(frontId))
        renderer.loadTexture(frontId, frontPath);
    if (!renderer.hasTexture(backId))
        renderer.loadTexture(backId, backPath);
}

void GameUI::drawPartyList(const Player &player, int selected) {
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                            TDT4102::Color{140, 155, 190});

    const auto &party = player.getParty();
    int scale = PIXEL_SCALE;
    int slotHeight = 24 * scale;
    int gap = 4 * scale;
    int startY = 12;

    int sy = startY;
    int partyCount = static_cast<int>(party.size());
    for (int i = 0; i < partyCount; ++i) {

        bool isSelected = i == selected;

        TDT4102::Color bgColor = isSelected ? TDT4102::Color{200, 215, 255}
                                            : TDT4102::Color{230, 235, 250};

        renderer.drawFilledRect(20, sy, WINDOW_WIDTH - 40, slotHeight, bgColor);
        renderer.drawRect(20, sy, WINDOW_WIDTH - 40, slotHeight,
                          TDT4102::Color::transparent, TDT4102::Color::black);

        const Daemon &c = party[static_cast<std::size_t>(i)];

        if (isSelected)
            drawSelectionArrow(28, sy + 6 * scale, scale);

        spriteFont.drawText(renderer, c.getNickname(), 28 + 6 * scale,
                            sy + 2 * scale, scale);

        int lvlX = WINDOW_WIDTH / 2 - 30 * scale;
        spriteFont.drawText(renderer, "Lv" + std::to_string(c.getLevel()), lvlX,
                            sy + 2 * scale, scale);

        int hpBarX = WINDOW_WIDTH / 2 + 10 * scale;
        int hpBarW = 48 * scale;
        drawSpriteHPBar(hpBarX, sy + 4 * scale, hpBarW, c.getCurrentHP(),
                        c.getMaxHP(), scale);

        spriteFont.drawText(renderer,
                            std::to_string(c.getCurrentHP()) + "-" +
                                std::to_string(c.getMaxHP()),
                            hpBarX, sy + 9 * scale, scale);

        std::string typeName = StringUtils::capitalize(
            elementTypeName(c.getSpecies().primaryType));
        spriteFont.drawText(renderer, typeName, 28 + 6 * scale, sy + 14 * scale,
                            scale - 1);

        sy += slotHeight + gap;
    }
}

void GameUI::drawSummaryScreen(const Daemon &daemon, const Pokedex &pokedex,
                               int page) {
    loadBattleAssets();
    int scale = PIXEL_SCALE;
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                            TDT4102::Color{150, 160, 195});

    // Daemon sprite (front)
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
    spriteFont.drawText(
        renderer, lvlStr,
        24 + spriteFont.getTextWidth(daemon.getNickname(), scale) + 8 * scale,
        headerY, scale);

    int subY = headerY + 18 * scale;
    if (daemon.getNickname() != daemon.getSpecies().name) {
        spriteFont.drawText(renderer, daemon.getSpecies().name, 24, subY,
                            scale - 1);
    }

    int typeY = subY + 14 * scale;
    std::string primaryType = StringUtils::capitalize(
        elementTypeName(daemon.getSpecies().primaryType));
    std::string secondaryType = StringUtils::capitalize(
        elementTypeName(daemon.getSpecies().secondaryType));

    TDT4102::Color typeBg{90, 100, 145};
    int typeBoxH = 14 * scale;
    int typeTextY = typeY + 2 * scale;

    int tw1 = spriteFont.getTextWidth(primaryType, scale - 1) + 6 * scale;
    renderer.drawFilledRect(24, typeY, tw1, typeBoxH, typeBg);
    spriteFont.drawText(renderer, primaryType, 24 + 3 * scale, typeTextY,
                        scale - 1);

    if (daemon.getSpecies().primaryType != daemon.getSpecies().secondaryType) {
        int tw2 = spriteFont.getTextWidth(secondaryType, scale - 1) + 6 * scale;
        renderer.drawFilledRect(24 + tw1 + 4 * scale, typeY, tw2, typeBoxH,
                                typeBg);
        spriteFont.drawText(renderer, secondaryType,
                            24 + tw1 + 4 * scale + 3 * scale, typeTextY,
                            scale - 1);
    }

    int divY = typeY + typeBoxH + 4 * scale;
    renderer.drawFilledRect(20, divY, WINDOW_WIDTH / 2, 2,
                            TDT4102::Color{120, 130, 170});

    if (page == 0) {
        int infoY = divY + 4 * scale;
        spriteFont.drawText(renderer, "HP", 24, infoY, scale);
        int hpBarX = 54 + 16 * scale;
        int hpBarW = 48 * scale;
        drawSpriteHPBar(hpBarX, infoY + 4 * scale, hpBarW,
                        daemon.getCurrentHP(), daemon.getMaxHP(), scale);
        spriteFont.drawText(renderer,
                            std::to_string(daemon.getCurrentHP()) + "-" +
                                std::to_string(daemon.getMaxHP()),
                            hpBarX + hpBarW + 4 * scale, infoY, scale);

        int expY = infoY + 14 * scale;
        spriteFont.drawText(renderer, "EXP", 24, expY, scale);
        int expBarX = 54 + 16 * scale;
        int expBarW = 48 * scale;
        int expNeeded = daemon.getExpNeeded();
        drawSpriteEXPBar(expBarX, expY + 4 * scale, expBarW, daemon.getExp(),
                         expNeeded, scale);
        spriteFont.drawText(renderer,
                            std::to_string(daemon.getExp()) + "-" +
                                std::to_string(expNeeded),
                            expBarX + expBarW + 4 * scale, expY, scale);

        int movesHeaderY = expY + 16 * scale;
        renderer.drawFilledRect(20, movesHeaderY, WINDOW_WIDTH - 40, 2,
                                TDT4102::Color{120, 130, 170});
        int moveLabelY = movesHeaderY + 4 * scale;
        spriteFont.drawText(renderer, "MOVES", 24, moveLabelY, scale);

        int moveStartY = moveLabelY + 14 * scale;
        // Calculate move slot height to fit remaining space
        int availableH =
            (WINDOW_HEIGHT - 16 * scale) - moveStartY; // footer at bottom
        int moveSlotH = availableH / 4;

        const auto &moves = daemon.getMoves();
        int my = moveStartY;
        for (int i = 0; i < 4; ++i) {

            // Subtle alternating row background
            if (i % 2 == 0)
                renderer.drawFilledRect(24, my, WINDOW_WIDTH - 48,
                                        moveSlotH - 2,
                                        TDT4102::Color{135, 145, 180});

            if (moves[static_cast<std::size_t>(i)].moveId < 0) {
                spriteFont.drawText(renderer, "---", 36, my + 2 * scale, scale);
                continue;
            }

            const MoveData &moveData =
                pokedex.getMove(moves[static_cast<std::size_t>(i)].moveId);

            spriteFont.drawText(renderer, moveData.name, 36, my + 2 * scale,
                                scale);

            std::string mType =
                StringUtils::capitalize(elementTypeName(moveData.type));
            spriteFont.drawText(renderer, mType, WINDOW_WIDTH / 2 - 10 * scale,
                                my + 2 * scale, scale - 1);

            std::string pp =
                "PP " +
                std::to_string(moves[static_cast<std::size_t>(i)].currentPP) +
                "-" + std::to_string(moves[static_cast<std::size_t>(i)].maxPP);
            int ppX = WINDOW_WIDTH - 24 - spriteFont.getTextWidth(pp, scale);
            spriteFont.drawText(renderer, pp, ppX, my + 2 * scale, scale);

            my += moveSlotH;
        }
    } else {
        int infoY = divY + 4 * scale;
        spriteFont.drawText(renderer, "STATS", 24, infoY, scale);

        static const std::string statNames[] = {"HP",  "Atk", "Def",
                                                "SpA", "SpD", "Spe"};
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
                renderer.drawFilledRect(barX, sy + 3 * scale, barW, barH,
                                        barColor);
            }
        }
    }

    int footerY = WINDOW_HEIGHT - 16 * scale;
    std::string pageText = (page == 0) ? "Info  LR:Stats" : "Stats  LR:Info";
    int pageTextX =
        (WINDOW_WIDTH - spriteFont.getTextWidth(pageText, scale - 1)) / 2;
    spriteFont.drawText(renderer, pageText, pageTextX, footerY, scale - 1);

    spriteFont.drawText(renderer, "B:Back", 24, footerY, scale - 1);
}

void GameUI::drawBagScreen(const Player &player, const Pokedex &pokedex,
                           int selected) {
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                            TDT4102::Color{170, 155, 130});

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

        TDT4102::Color bgColor = isSelected ? TDT4102::Color{255, 230, 180}
                                            : TDT4102::Color{240, 220, 200};

        renderer.drawFilledRect(20, sy, WINDOW_WIDTH - 40, slotHeight, bgColor);
        renderer.drawRect(20, sy, WINDOW_WIDTH - 40, slotHeight,
                          TDT4102::Color::transparent, TDT4102::Color::black);

        const ItemData &item =
            pokedex.getItem(inventory[static_cast<std::size_t>(i)].itemId);

        if (isSelected)
            drawSelectionArrow(28, sy + 4 * scale, scale);

        spriteFont.drawText(renderer, item.name, 28 + 6 * scale, sy + 4, scale);

        std::string qtyStr =
            "x" +
            std::to_string(inventory[static_cast<std::size_t>(i)].quantity);
        int qtyX = WINDOW_WIDTH - 60 - spriteFont.getTextWidth(qtyStr, scale);
        spriteFont.drawText(renderer, qtyStr, qtyX, sy + 4, scale);

        sy += slotHeight + 8;
    }

    // Description of selected item at the bottom
    if (selected >= 0 && selected < static_cast<int>(inventory.size())) {
        const ItemData &item = pokedex.getItem(
            inventory[static_cast<std::size_t>(selected)].itemId);
        int descY = WINDOW_HEIGHT - 30 * scale;
        drawNarrowTextBar(20, descY, 180, scale);
        spriteFont.drawText(renderer, item.description, 28 + 4 * scale,
                            descY + 5 * scale, scale);
    }
}

void GameUI::navigateVertical(int &selected, int count) {
    if (count <= 0)
        return;

    if (selected < 0 || selected >= count)
        selected = (selected % count + count) % count;

    Direction dir;
    bool dirHeld = input.getMovementDirection(dir);

    if (dirHeld && (dir == Direction::up || dir == Direction::down)) {
        if (!menuDirHeld || dir != menuLastDir) {
            if (dir == Direction::up)
                selected = (selected - 1 + count) % count;
            else
                selected = (selected + 1) % count;
            menuRepeatTimer = 0;
            menuFirstRepeat = true;
            menuDirHeld = true;
            menuLastDir = dir;
        } else {
            menuRepeatTimer++;
            int delay = menuFirstRepeat ? menuInitialDelay : menuRepeatDelay;
            if (menuRepeatTimer >= delay) {
                if (dir == Direction::up)
                    selected = (selected - 1 + count) % count;
                else
                    selected = (selected + 1) % count;
                menuRepeatTimer = 0;
                menuFirstRepeat = false;
            }
        }
    } else {
        menuDirHeld = false;
        menuRepeatTimer = 0;
        menuFirstRepeat = true;
    }
}

void GameUI::navigate2x2(int &sel) {
    if (input.isRightHeld() && (sel % 2) == 0)
        sel += 1;
    else if (input.isLeftHeld() && (sel % 2) == 1)
        sel -= 1;
    else if (input.isDownHeld() && sel < 2)
        sel += 2;
    else if (input.isUpHeld() && sel >= 2)
        sel -= 2;
}

void GameUI::setDialogueText(const std::string &text) {
    if (text != typewriterFullText) {
        typewriterFullText = text;
        typewriterCharsRevealed = 0;
        typewriterFrameCounter = 0;
        typewriterIndicatorTimer = 0;
    }
}

bool GameUI::updateTypewriter(const bool inputConfirm) {
    if (isTextFullyRevealed()) {
        // Advance the indicator animation timer continuously
        typewriterIndicatorTimer++;
        return inputConfirm;
    }

    // Determine speed: fast if confirm or cancel is held
    bool fast = input.isConfirmHeld() || input.isCancelHeld();
    int speed = fast ? typewriterFastSpeed : typewriterSpeed;

    typewriterFrameCounter++;
    if (typewriterFrameCounter >= speed) {
        typewriterFrameCounter = 0;
        typewriterCharsRevealed++;
        if (typewriterCharsRevealed >= typewriterFullText.size()) {
            typewriterCharsRevealed = typewriterFullText.size();
            typewriterIndicatorTimer = 0;
        }
    }
    if (!inputConfirm)
        return false;

    if (!isTextFullyRevealed()) {
        revealAllText();
        return false;
    }
    return true;
}

bool GameUI::isTextFullyRevealed() const {
    return typewriterCharsRevealed >= typewriterFullText.size();
}

void GameUI::revealAllText() {
    typewriterCharsRevealed = typewriterFullText.size();
    typewriterIndicatorTimer = 0;
}

void GameUI::startDialogue(const std::string &speaker,
                           const std::vector<std::string> &lines) {
    dialogueSpeaker = speaker;
    dialogueLines = lines;
    dialogueLineIndex = 0;
    if (!lines.empty())
        setDialogueText(lines[0]);
}

bool GameUI::advanceDialogueLine() {
    dialogueLineIndex++;
    if (isDialogueActive()) {
        setDialogueText(
            dialogueLines[static_cast<std::size_t>(dialogueLineIndex)]);
        return true;
    }
    return false;
}

bool GameUI::isDialogueActive() const {
    return dialogueLineIndex >= 0 &&
           dialogueLineIndex < static_cast<int>(dialogueLines.size());
}

const std::string &GameUI::getCurrentDialogueLine() const {
    return dialogueLines[static_cast<std::size_t>(dialogueLineIndex)];
}

const std::string &GameUI::getDialogueSpeaker() const {
    return dialogueSpeaker;
}

bool GameUI::tickHPAnimation(int targetPlayerHP, int targetOpponentHP,
                             int maxPlayerHP, int maxOpponentHP) {
    int playerStep = std::max(1, maxPlayerHP / 15);
    int opponentStep = std::max(1, maxOpponentHP / 15);

    if (playerDisplayHP > targetPlayerHP)
        playerDisplayHP =
            std::max(targetPlayerHP, playerDisplayHP - playerStep);
    else if (playerDisplayHP < targetPlayerHP)
        playerDisplayHP =
            std::min(targetPlayerHP, playerDisplayHP + playerStep);

    if (opponentDisplayHP > targetOpponentHP)
        opponentDisplayHP =
            std::max(targetOpponentHP, opponentDisplayHP - opponentStep);
    else if (opponentDisplayHP < targetOpponentHP)
        opponentDisplayHP =
            std::min(targetOpponentHP, opponentDisplayHP + opponentStep);

    return (playerDisplayHP == targetPlayerHP &&
            opponentDisplayHP == targetOpponentHP);
}

EXPTickResult GameUI::tickEXPAnimation(int targetEXP, int expNeeded) {
    static constexpr int EXP_ANIM_FRAMES = 120;
    int destination = std::min(targetEXP, expNeeded);

    if (expAnimStartEXP < 0)
        expAnimStartEXP = playerDisplayEXP;

    expAnimFrame++;

    if (expAnimFrame >= EXP_ANIM_FRAMES) {
        // Animation complete — snap to destination
        playerDisplayEXP = destination;
        expAnimFrame = 0;
        expAnimStartEXP = -1;

        if (playerDisplayEXP >= expNeeded)
            return EXPTickResult::filledBar;
        return EXPTickResult::reachedTarget;
    }

    // Linearly interpolate between start and destination
    playerDisplayEXP = expAnimStartEXP + (destination - expAnimStartEXP) *
                                             (expAnimFrame / EXP_ANIM_FRAMES);

    // Check if we've crossed the bar boundary mid-animation
    if (playerDisplayEXP >= expNeeded) {
        playerDisplayEXP = expNeeded;
        expAnimFrame = 0;
        expAnimStartEXP = -1;
        return EXPTickResult::filledBar;
    }

    return EXPTickResult::inProgress;
}

void GameUI::drawDialogueBox(const std::string &speaker,
                             const std::string &text) {
    // Update typewriter state for this text
    setDialogueText(text);

    int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    int scale = PIXEL_SCALE;

    drawTextBar(panelY);

    int textBarW = 252 * scale;
    int textBarH = 46 * scale;
    int textBarX = (WINDOW_WIDTH - textBarW) / 2;
    int textBarY = panelY + (UI_PANEL_HEIGHT - textBarH) / 2;

    int textX = textBarX + 12 * scale;
    int textY = textBarY + 6 * scale;

    int textMaxW = textBarW - 24 * scale;
    spriteFont.drawTextPartial(renderer, text, typewriterCharsRevealed, textX,
                               textY, scale, 1, textMaxW);

    // Bouncing continue indicator
    if (isTextFullyRevealed()) {
        int bouncePhase = typewriterIndicatorTimer % 20;
        int bounceOffset = (bouncePhase < 10) ? (bouncePhase / 3)
                                              : (3 - (bouncePhase - 10) / 3);

        int indicatorX = textBarX + textBarW - 12 * scale;
        int indicatorY =
            textBarY + textBarH - 14 * scale + bounceOffset * scale;

        spriteFont.drawContinueIndicator(renderer, indicatorX, indicatorY,
                                         scale);
    }

    // Speaker name badge
    if (!speaker.empty()) {
        int spkW = spriteFont.getTextWidth(speaker, scale) + 6 * scale;
        int spkH = 16 * scale + 4 * scale;
        int spkX = textBarX + 4 * scale;
        int spkY = textBarY - spkH - 2;

        renderer.drawFilledRect(spkX, spkY, spkW, spkH, TDT4102::Color::white);
        renderer.drawRect(spkX, spkY, spkW, spkH, TDT4102::Color::transparent,
                          TDT4102::Color::black);
        spriteFont.drawText(renderer, speaker, spkX + 3 * scale,
                            spkY + 2 * scale, scale);
    }
}

void GameUI::drawChoiceBox(const std::vector<std::string> &options,
                           int selected) {
    int scale = PIXEL_SCALE;
    // Auto-size width to fit the longest option
    int maxTextW = 0;
    for (const auto &opt : options) {
        int tw = spriteFont.getTextWidth(opt, scale);
        if (tw > maxTextW)
            maxTextW = tw;
    }
    int boxWidth = maxTextW + 10 * scale; // arrow (6*scale) + padding
    int itemHeight = 16 * scale + 8;      // glyph height * scale + padding
    int boxHeight = 16;
    int optionCount = static_cast<int>(options.size());
    for (int i = 0; i < optionCount; ++i) {
        boxHeight += itemHeight;
    }
    int boxX = WINDOW_WIDTH - boxWidth - 20;
    int boxY = WINDOW_HEIGHT - UI_PANEL_HEIGHT - boxHeight - 10;

    renderer.drawFilledRect(boxX, boxY, boxWidth, boxHeight,
                            TDT4102::Color{240, 245, 255});
    renderer.drawRect(boxX, boxY, boxWidth, boxHeight,
                      TDT4102::Color::transparent, TDT4102::Color{60, 70, 100});

    int oy = boxY + 8;
    for (int i = 0; i < optionCount; ++i) {
        if (i == selected)
            drawSelectionArrow(boxX + 2 * scale, oy + 4 * scale, scale);
        spriteFont.drawText(renderer, options[static_cast<std::size_t>(i)],
                            boxX + 6 * scale, oy, scale);
        oy += itemHeight;
    }
}

void GameUI::drawTextBox(int x, int y, int width, int height,
                         const std::string &text) {
    renderer.drawFilledRect(x, y, width, height, TDT4102::Color::white);
    renderer.drawRect(x, y, width, height, TDT4102::Color::transparent,
                      TDT4102::Color::black);
    spriteFont.drawText(renderer, text, x + 16, y + 14, PIXEL_SCALE);
}

void GameUI::drawTextBar(int panelY) {
    int scale = PIXEL_SCALE;
    int barW = 252 * scale;
    int barH = 46 * scale;
    int barX = (WINDOW_WIDTH - barW) / 2;
    int barY = panelY + (UI_PANEL_HEIGHT - barH) / 2;
    renderer.drawSpriteRaw("ui_text_bar", barX, barY, barW, barH);
}

void GameUI::drawNarrowTextBar(int x, int y, int srcW, int scale) {
    // text_bar.png is 252x46 source pixels
    // Draw the left edge, a tiled middle, and the right edge from the source
    constexpr int fullSrcW = 252;
    constexpr int srcH = 46;
    constexpr int edgeW = 8; // source pixels for each edge

    int clampedSrcW = std::max(srcW, edgeW * 2 + 1);
    if (clampedSrcW > fullSrcW)
        clampedSrcW = fullSrcW;

    int dstH = srcH * scale;
    int middleSrcW = clampedSrcW - edgeW * 2;

    // Left edge
    renderer.drawSpriteRegion("ui_text_bar", 0, 0, edgeW, srcH, x, y,
                              edgeW * scale, dstH);

    // Middle: tile from center of source bar
    int midSrcX = fullSrcW / 2 - middleSrcW / 2;
    renderer.drawSpriteRegion("ui_text_bar", midSrcX, 0, middleSrcW, srcH,
                              x + edgeW * scale, y, middleSrcW * scale, dstH);

    // Right edge
    renderer.drawSpriteRegion("ui_text_bar", fullSrcW - edgeW, 0, edgeW, srcH,
                              x + (edgeW + middleSrcW) * scale, y,
                              edgeW * scale, dstH);
}

// Pixel-art right-pointing arrow
void GameUI::drawSelectionArrow(int x, int y, int scale) {
    int s = scale;
    renderer.drawFilledRect(x, y + 0 * s, 1 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 1 * s, 2 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 2 * s, 3 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 3 * s, 2 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 4 * s, 1 * s, 1 * s, TDT4102::Color::black);
}

// Daemon info bars

void GameUI::drawOpponentInfoBar(const Daemon *opponentDaemon, int offsetX) {
    const int scale = PIXEL_SCALE;

    int oppPanelW = 122 * scale;
    int oppPanelH = 35 * scale;
    int oppPanelX = offsetX;
    int oppPanelY = 16;

    renderer.drawSpriteRaw("ui_opponent_info", oppPanelX, oppPanelY, oppPanelW,
                           oppPanelH);

    if (opponentDaemon) {
        int oppNameX = oppPanelX + 2 * scale;
        int oppNameY = oppPanelY + 9 * scale;
        spriteFont.drawText(renderer, opponentDaemon->getNickname(), oppNameX,
                            oppNameY, scale);

        int oppLvlX = oppPanelX + 83 * scale;
        int oppLvlY = oppPanelY + 12 * scale;
        spriteFont.drawBattleNumber(renderer, opponentDaemon->getLevel(),
                                    oppLvlX, oppLvlY, scale);

        int oppHPBarX = oppPanelX + 50 * scale;
        int oppHPBarY = oppPanelY + 24 * scale;
        int oppHPBarW = 48 * scale;
        drawSpriteHPBar(oppHPBarX, oppHPBarY, oppHPBarW, opponentDisplayHP,
                        opponentDaemon->getMaxHP(), scale);
    }
}
void GameUI::drawPlayerInfoBar(const Daemon *playerDaemon, int offsetX) {
    const int scale = PIXEL_SCALE;
    int battleH = WINDOW_HEIGHT - UI_PANEL_HEIGHT;

    int plyPanelW = 128 * scale;
    int plyPanelH = 47 * scale;
    int plyPanelX = WINDOW_WIDTH - plyPanelW + offsetX;
    int plyPanelY = battleH - plyPanelH - 10;

    renderer.drawSpriteRaw("ui_player_info", plyPanelX, plyPanelY, plyPanelW,
                           plyPanelH);

    if (playerDaemon) {
        int plyNameX = plyPanelX + 18 * scale;
        int plyNameY = plyPanelY + 10 * scale;
        spriteFont.drawText(renderer, playerDaemon->getNickname(), plyNameX,
                            plyNameY, scale);

        int plyLvlX = plyPanelX + 105 * scale;
        int plyLvlY = plyPanelY + 12 * scale;
        spriteFont.drawBattleNumber(renderer, playerDaemon->getLevel(), plyLvlX,
                                    plyLvlY, scale);

        int plyHPBarX = plyPanelX + 72 * scale;
        int plyHPBarY = plyPanelY + 25 * scale;
        int plyHPBarW = 48 * scale;
        drawSpriteHPBar(plyHPBarX, plyHPBarY, plyHPBarW, playerDisplayHP,
                        playerDaemon->getMaxHP(), scale);

        int hpNumX = plyPanelX + 92 * scale;
        int hpNumY = plyPanelY + 32 * scale;
        int hpPad = 3 * scale;
        spriteFont.drawBattleNumber(renderer, playerDisplayHP, hpNumX - hpPad,
                                    hpNumY, scale, true);
        spriteFont.drawBattleNumber(renderer, playerDaemon->getMaxHP(),
                                    hpNumX + hpPad, hpNumY, scale);

        int expBarX = plyPanelX + 24 * scale;
        int expBarY = plyPanelY + 42 * scale;
        int expBarW = 96 * scale;
        int expNeeded = playerDaemon->getExpNeeded();
        drawSpriteEXPBar(expBarX, expBarY, expBarW, playerDisplayEXP, expNeeded,
                         scale);
    }
}

void GameUI::drawOpponentDaemon(const Daemon *opponentDaemon, int offsetX,
                                int offsetY) {
    int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getOpponentBaseGeometry();

    if (opponentDaemon) {
        const std::string &opponentSpeciesName =
            opponentDaemon->getSpecies().name;
        loadDaemonSprite(opponentSpeciesName);
        std::string oppSpriteId = "daemon_" + opponentDaemon->getSpecies().name;
        TDT4102::Image &oppImg = renderer.getTexture(oppSpriteId);
        int oppSprW = oppImg.width * scale;
        int oppSprH = oppImg.height * scale;
        int oppSprX = baseX + baseW / 2 - oppSprW / 2 + offsetX;
        int oppSprY = baseY - oppSprH + baseH - 10 * scale + offsetY;
        renderer.drawSpriteRaw(oppSpriteId, oppSprX, oppSprY, oppSprW, oppSprH);
    }
}
void GameUI::drawPlayerDaemon(const Daemon *playerDaemon, int offsetX,
                              int offsetY) {
    const int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getPlayerBaseGeometry();

    if (playerDaemon) {
        const std::string &playerSpeciesName = playerDaemon->getSpecies().name;
        loadDaemonSprite(playerSpeciesName);
        std::string plySpriteId =
            "daemon_" + playerDaemon->getSpecies().name + "_back";
        TDT4102::Image &plyImg = renderer.getTexture(plySpriteId);
        int plySprW = plyImg.width * scale;
        int plySprH = plyImg.height * scale;
        int plySprX = baseX + baseW / 2 - plySprW / 2 + offsetX;
        int plySprY = baseY - plySprH + baseH + offsetY;
        renderer.drawSpriteRaw(plySpriteId, plySprX, plySprY, plySprW, plySprH);
    }
}

// --- Player back spritesheet (3x3 grid of 80x80, 5 frames used) ---
void GameUI::drawPlayerBackSprite(int x, int y, int dstW, int dstH, int frame) {
    constexpr int FRAME_W = 80;
    constexpr int FRAME_H = 80;
    constexpr int COLS = 3;
    constexpr int TOTAL_FRAMES = 5;
    int f = frame % TOTAL_FRAMES;
    int srcX = (f % COLS) * FRAME_W;
    int srcY = (f / COLS) * FRAME_H;
    renderer.drawSpriteRegion("player_back", srcX, srcY, FRAME_W, FRAME_H, x, y,
                              dstW, dstH);
}

void GameUI::drawBattleBackground() {
    int battleH = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, battleH,
                            TDT4102::Color{200, 220, 200});
}

GameUI::BattleBaseGeometry GameUI::getPlayerBaseGeometry() const {
    const int scale = PIXEL_SCALE;
    int battleH = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    int w = 256 * scale;
    int h = 32 * scale;
    return {-60 * scale, battleH - h, w, h};
}

GameUI::BattleBaseGeometry GameUI::getOpponentBaseGeometry() const {
    const int scale = PIXEL_SCALE;
    int w = 128 * scale;
    int h = 64 * scale;
    return {WINDOW_WIDTH - w - 60, 120, w, h};
}

void GameUI::drawPlayerBase() {
    auto [x, y, w, h] = getPlayerBaseGeometry();
    renderer.drawSpriteRaw("ui_player_base", x, y, w, h);
}

void GameUI::drawOpponentBase(int offsetX) {
    auto [x, y, w, h] = getOpponentBaseGeometry();
    renderer.drawSpriteRaw("ui_opponent_base", x + offsetX, y, w, h);
}

void GameUI::drawOpponentTrainer(const NPC *opponent, int offsetX) {
    const int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getOpponentBaseGeometry();

    // Draw a silhouette rectangle as trainer placeholder
    int trainerW = 40 * scale;
    int trainerH = 56 * scale;
    int trainerX = baseX + baseW / 2 - trainerW / 2 + offsetX;
    int trainerY = baseY + baseH - trainerH - 10 * scale;
    renderer.drawFilledRect(trainerX, trainerY, trainerW, trainerH,
                            TDT4102::Color{60, 60, 80});
    if (opponent) {
        spriteFont.drawText(renderer, opponent->getName(), trainerX + 2 * scale,
                            trainerY + trainerH / 2, scale - 1);
    }
}

void GameUI::drawPlayerBackOnBase(int offsetX, int frame) {
    const int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getPlayerBaseGeometry();

    int backW = 80 * scale;
    int backH = 80 * scale;
    int backX = baseX + baseW / 2 - backW / 2 + offsetX;
    int backY = baseY - backH + baseH;
    drawPlayerBackSprite(backX, backY, backW, backH, frame);
}

void GameUI::drawPlayerSendOutPhase(const Daemon *playerDaemon, float t) {
    // Player back sprite slides out to the left (first half)
    int playerSlideOut = static_cast<int>(-WINDOW_WIDTH * t);
    int throwFrame = battleIntroFrame / 4;
    if (t < 0.5f)
        drawPlayerBackOnBase(playerSlideOut, throwFrame);

    // Player Daemon slides in from off-screen left
    int daemonSlideIn = static_cast<int>(-WINDOW_WIDTH * (1.0f - t));
    drawPlayerDaemon(playerDaemon, daemonSlideIn);

    // Player info bar slides in from right
    int infoSlideIn = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
    drawPlayerInfoBar(playerDaemon, infoSlideIn);
}

// HP bar sprite: hp_bar.png rows — 0-2 green, 3-5 yellow, 6-8 red, 9-11 empty
void GameUI::drawSpriteHPBar(int x, int y, int width, int currentHP, int maxHP,
                             int scale) {
    float ratio =
        (maxHP > 0) ? static_cast<float>(currentHP) / static_cast<float>(maxHP)
                    : 0.0f;
    int filledWidth = static_cast<int>(static_cast<float>(width) * ratio);

    int emptySrcY = 9;
    int fillSrcY;
    if (ratio > 0.5f)
        fillSrcY = 0;
    else if (ratio > 0.2f)
        fillSrcY = 3;
    else
        fillSrcY = 6;

    int barH = 3 * scale;

    for (int dx = 0; dx < width; dx += scale) {
        int tileW = std::min(scale, width - dx);
        renderer.drawSpriteRegion("ui_hp_bar", 0, emptySrcY, 1, 3, x + dx, y,
                                  tileW, barH);
    }

    if (filledWidth > 0) {
        for (int dx = 0; dx < filledWidth; dx += scale) {
            int tileW = std::min(scale, filledWidth - dx);
            renderer.drawSpriteRegion("ui_hp_bar", 0, fillSrcY, 1, 3, x + dx, y,
                                      tileW, barH);
        }
    }
}

// EXP bar sprite: exp_bar.png rows — 0-1 empty, 2-3 filled
void GameUI::drawSpriteEXPBar(int x, int y, int width, int currentEXP,
                              int maxEXP, int scale) {
    int filledWidth = 0;
    if (maxEXP > 0)
        filledWidth = std::min(width, currentEXP * width / maxEXP);

    int barH = 2 * scale;

    for (int dx = 0; dx < width; dx += scale) {
        int tileW = std::min(scale, width - dx);
        renderer.drawSpriteRegion("ui_exp_bar", 0, 0, 1, 2, x + dx, y, tileW,
                                  barH);
    }

    if (filledWidth > 0) {
        for (int dx = 0; dx < filledWidth; dx += scale) {
            int tileW = std::min(scale, filledWidth - dx);
            renderer.drawSpriteRegion("ui_exp_bar", 0, 2, 1, 2, x + dx, y,
                                      tileW, barH);
        }
    }
}
