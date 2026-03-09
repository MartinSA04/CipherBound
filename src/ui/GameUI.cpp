#include "GameUI.h"
#include <algorithm>
#include <cctype>

GameUI::GameUI()
    : renderer(),
      input(renderer.getWindow()),
      overworldRenderer(renderer),
      spriteFont(),
      currentScreen(ScreenType::title)
{
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

// --- Overworld ---

void GameUI::drawOverworld(const Map &map, const Player &player,
                           const std::vector<std::shared_ptr<NPC>> &npcs)
{
    overworldRenderer.render(map, player, npcs);
}

// --- Battle UI ---

void GameUI::loadBattleAssets()
{
    if (battleAssetsLoaded)
        return;

    renderer.loadTexture("ui_player_info", "assets/sprites/ui/player_daemon_info.png");
    renderer.loadTexture("ui_opponent_info", "assets/sprites/ui/opponent_daemon_info.png");
    renderer.loadTexture("ui_hp_bar", "assets/sprites/ui/hp_bar.png");
    renderer.loadTexture("ui_exp_bar", "assets/sprites/ui/exp_bar.png");
    renderer.loadTexture("ui_player_base", "assets/sprites/ui/player_battle_base.png");
    renderer.loadTexture("ui_opponent_base", "assets/sprites/ui/opponent_battle_base.png");
    renderer.loadTexture("player_back", "assets/sprites/player/player_back.png");

    battleAssetsLoaded = true;
}

void GameUI::loadDaemonSprite(const std::string &speciesName)
{
    std::string frontId = "daemon_" + speciesName;
    std::string backId = "daemon_" + speciesName + "_back";
    std::string frontPath = "assets/sprites/daemons/" + speciesName + ".png";
    std::string backPath = "assets/sprites/daemons/" + speciesName + "_back.png";

    if (!renderer.hasTexture(frontId))
        renderer.loadTexture(frontId, frontPath);
    if (!renderer.hasTexture(backId))
        renderer.loadTexture(backId, backPath);
}

// --- Battle UI (high-level draw functions moved to BattleMode) ---

// --- Menus ---

// --- Main menu (moved to MenuMode) ---

void GameUI::drawPartyList(const Player &player, int selected)
{
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{40, 40, 80});

    const auto &party = player.getParty();
    int scale = PIXEL_SCALE;
    int slotHeight = 16 * scale + 8;
    int startY = 20;

    for (int i = 0; i < static_cast<int>(party.size()); ++i)
    {
        int sy = startY + i * (slotHeight + 8);

        TDT4102::Color bgColor = (i == selected)
                                     ? TDT4102::Color{200, 215, 255}
                                     : TDT4102::Color{230, 235, 250};

        renderer.drawFilledRect(20, sy, WINDOW_WIDTH - 40, slotHeight, bgColor);
        renderer.drawRect(20, sy, WINDOW_WIDTH - 40, slotHeight,
                          TDT4102::Color::transparent, TDT4102::Color::black);

        const Daemon &c = party[i];

        if (i == selected)
            drawSelectionArrow(28, sy + 4 * scale, scale);

        spriteFont.drawText(renderer, c.getNickname(), 28 + 6 * scale, sy + 4, scale);

        int lvlX = WINDOW_WIDTH / 2 - 30 * scale;
        spriteFont.drawText(renderer, "Lv" + std::to_string(c.getLevel()), lvlX, sy + 4, scale);

        int hpBarX = WINDOW_WIDTH / 2 + 10 * scale;
        int hpBarW = 48 * scale;
        drawSpriteHPBar(hpBarX, sy + 6 * scale, hpBarW, c.getCurrentHP(), c.getMaxHP(), scale);

        spriteFont.drawText(renderer, std::to_string(c.getCurrentHP()) + "-" + std::to_string(c.getMaxHP()),
                            hpBarX, sy + 10 * scale, scale);
    }
}

void GameUI::drawBagScreen(const Player &player, const Pokedex &pokedex, int selected)
{
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{80, 60, 40});

    int scale = PIXEL_SCALE;
    spriteFont.drawText(renderer, "BAG", 28, 10, scale);

    const auto &inventory = player.getInventory();

    if (inventory.empty())
    {
        spriteFont.drawText(renderer, "No items", 28, 60, scale);
        return;
    }

    int slotHeight = 16 * scale + 8;
    int startY = 10 + 20 * scale;

    for (int i = 0; i < static_cast<int>(inventory.size()); ++i)
    {
        int sy = startY + i * (slotHeight + 8);

        TDT4102::Color bgColor = (i == selected)
                                     ? TDT4102::Color{255, 230, 180}
                                     : TDT4102::Color{240, 220, 200};

        renderer.drawFilledRect(20, sy, WINDOW_WIDTH - 40, slotHeight, bgColor);
        renderer.drawRect(20, sy, WINDOW_WIDTH - 40, slotHeight,
                          TDT4102::Color::transparent, TDT4102::Color::black);

        const ItemData &item = pokedex.getItem(inventory[i].itemId);

        if (i == selected)
            drawSelectionArrow(28, sy + 4 * scale, scale);

        spriteFont.drawText(renderer, item.name, 28 + 6 * scale, sy + 4, scale);

        // Quantity on the right
        std::string qtyStr = "x" + std::to_string(inventory[i].quantity);
        int qtyX = WINDOW_WIDTH - 60 - static_cast<int>(qtyStr.size()) * 8 * scale;
        spriteFont.drawText(renderer, qtyStr, qtyX, sy + 4, scale);
    }

    // Description of selected item at the bottom
    if (selected >= 0 && selected < static_cast<int>(inventory.size()))
    {
        const ItemData &item = pokedex.getItem(inventory[selected].itemId);
        int descY = WINDOW_HEIGHT - 30 * scale;
        drawNarrowTextBar(20, descY, 180, scale);
        spriteFont.drawText(renderer, item.description, 28 + 4 * scale, descY + 5 * scale, scale);
    }
}

// --- PC Box screen (moved to PCBoxMode) ---

void GameUI::navigateVertical(int &selected, int count)
{
    if (count <= 0)
        return;
    Direction dir;
    bool dirHeld = input.getMovementDirection(dir);

    if (dirHeld && (dir == Direction::up || dir == Direction::down))
    {
        if (!menuDirHeld || dir != menuLastDir)
        {
            if (dir == Direction::up)
                selected = (selected - 1 + count) % count;
            else
                selected = (selected + 1) % count;
            menuRepeatTimer = 0;
            menuFirstRepeat = true;
            menuDirHeld = true;
            menuLastDir = dir;
        }
        else
        {
            menuRepeatTimer++;
            int delay = menuFirstRepeat ? menuInitialDelay : menuRepeatDelay;
            if (menuRepeatTimer >= delay)
            {
                if (dir == Direction::up)
                    selected = (selected - 1 + count) % count;
                else
                    selected = (selected + 1) % count;
                menuRepeatTimer = 0;
                menuFirstRepeat = false;
            }
        }
    }
    else
    {
        menuDirHeld = false;
        menuRepeatTimer = 0;
        menuFirstRepeat = true;
    }
}

void GameUI::navigate2x2(int &sel)
{
    if (input.isRightHeld() && (sel % 2) == 0)
        sel += 1;
    else if (input.isLeftHeld() && (sel % 2) == 1)
        sel -= 1;
    else if (input.isDownHeld() && sel < 2)
        sel += 2;
    else if (input.isUpHeld() && sel >= 2)
        sel -= 2;
}

// --- Dialogue ---

void GameUI::setDialogueText(const std::string &text)
{
    if (text != typewriterFullText)
    {
        typewriterFullText = text;
        typewriterCharsRevealed = 0;
        typewriterFrameCounter = 0;
        typewriterIndicatorTimer = 0;
    }
}

bool GameUI::updateTypewriter(const bool inputConfirm)
{
    if (isTextFullyRevealed())
    {
        // Advance the indicator animation timer continuously
        typewriterIndicatorTimer++;
        return inputConfirm;
    }

    // Determine speed: fast if confirm or cancel is held
    bool fast = input.isConfirmHeld() || input.isCancelHeld();
    int speed = fast ? typewriterFastSpeed : typewriterSpeed;

    typewriterFrameCounter++;
    if (typewriterFrameCounter >= speed)
    {
        typewriterFrameCounter = 0;
        typewriterCharsRevealed++;
        if (typewriterCharsRevealed >= static_cast<int>(typewriterFullText.size()))
        {
            typewriterCharsRevealed = static_cast<int>(typewriterFullText.size());
            typewriterIndicatorTimer = 0;
        }
    }
    if (!inputConfirm)
        return false;

    if (!isTextFullyRevealed())
    {
        revealAllText();
        return false;
    }
    return true;
}

bool GameUI::isTextFullyRevealed() const
{
    return typewriterCharsRevealed >= static_cast<int>(typewriterFullText.size());
}

void GameUI::revealAllText()
{
    typewriterCharsRevealed = static_cast<int>(typewriterFullText.size());
    typewriterIndicatorTimer = 0;
}

// --- Multi-line dialogue management ---

void GameUI::startDialogue(const std::string &speaker, const std::vector<std::string> &lines)
{
    dialogueSpeaker = speaker;
    dialogueLines = lines;
    dialogueLineIndex = 0;
    if (!lines.empty())
        setDialogueText(lines[0]);
}

bool GameUI::advanceDialogueLine()
{
    dialogueLineIndex++;
    if (dialogueLineIndex < static_cast<int>(dialogueLines.size()))
    {
        setDialogueText(dialogueLines[dialogueLineIndex]);
        return true;
    }
    return false;
}

bool GameUI::isDialogueActive() const
{
    return dialogueLineIndex < static_cast<int>(dialogueLines.size());
}

const std::string &GameUI::getCurrentDialogueLine() const
{
    return dialogueLines[dialogueLineIndex];
}

const std::string &GameUI::getDialogueSpeaker() const
{
    return dialogueSpeaker;
}

// --- HP/EXP animation tick helpers ---

bool GameUI::tickHPAnimation(int targetPlayerHP, int targetOpponentHP,
                             int maxPlayerHP, int maxOpponentHP)
{
    int playerStep = std::max(1, maxPlayerHP / 30);
    int opponentStep = std::max(1, maxOpponentHP / 30);

    if (playerDisplayHP > targetPlayerHP)
        playerDisplayHP = std::max(targetPlayerHP, playerDisplayHP - playerStep);
    else if (playerDisplayHP < targetPlayerHP)
        playerDisplayHP = std::min(targetPlayerHP, playerDisplayHP + playerStep);

    if (opponentDisplayHP > targetOpponentHP)
        opponentDisplayHP = std::max(targetOpponentHP, opponentDisplayHP - opponentStep);
    else if (opponentDisplayHP < targetOpponentHP)
        opponentDisplayHP = std::min(targetOpponentHP, opponentDisplayHP + opponentStep);

    return (playerDisplayHP == targetPlayerHP && opponentDisplayHP == targetOpponentHP);
}

EXPTickResult GameUI::tickEXPAnimation(int targetEXP, int expNeeded)
{
    int expStep = std::max(1, expNeeded / 30);
    playerDisplayEXP += expStep;

    if (playerDisplayEXP >= expNeeded)
        return EXPTickResult::filledBar;

    if (playerDisplayEXP >= targetEXP)
    {
        playerDisplayEXP = targetEXP;
        return EXPTickResult::reachedTarget;
    }
    return EXPTickResult::inProgress;
}

// --- Title screen ---

// --- Title screen (moved to TitleScreenMode) ---

void GameUI::drawDialogueBox(const std::string &speaker, const std::string &text)
{
    // Update typewriter state for this text
    setDialogueText(text);

    int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    int scale = PIXEL_SCALE;

    // Draw text bar background
    drawTextBar(panelY);

    int textBarW = 252 * scale;
    int textBarH = 46 * scale;
    int textBarX = (WINDOW_WIDTH - textBarW) / 2;
    int textBarY = panelY + (UI_PANEL_HEIGHT - textBarH) / 2;

    int textX = textBarX + 12 * scale;
    int textY = textBarY + 6 * scale;

    // Draw text with typewriter effect using sprite font (word-wrapped)
    int textMaxW = textBarW - 24 * scale; // 12px padding on each side
    spriteFont.drawTextPartial(renderer, text, typewriterCharsRevealed,
                               textX, textY, scale, 1, textMaxW);

    // Draw bouncing continue indicator when text is fully revealed
    if (isTextFullyRevealed())
    {
        int bouncePhase = typewriterIndicatorTimer % 40;
        int bounceOffset = (bouncePhase < 20) ? (bouncePhase / 5) : (4 - (bouncePhase - 20) / 5);

        int indicatorX = textBarX + textBarW - 12 * scale;
        int indicatorY = textBarY + textBarH - 14 * scale + bounceOffset * scale;

        spriteFont.drawContinueIndicator(renderer, indicatorX, indicatorY, scale);
    }

    // Speaker name badge using sprite font
    if (!speaker.empty())
    {
        int spkW = spriteFont.getTextWidth(speaker, scale) + 6 * scale;
        int spkH = 16 * scale + 4 * scale;
        int spkX = textBarX + 4 * scale;
        int spkY = textBarY - spkH - 2;

        renderer.drawFilledRect(spkX, spkY, spkW, spkH, TDT4102::Color::white);
        renderer.drawRect(spkX, spkY, spkW, spkH,
                          TDT4102::Color::transparent, TDT4102::Color::black);
        spriteFont.drawText(renderer, speaker, spkX + 3 * scale, spkY + 2 * scale, scale);
    }
}

void GameUI::drawChoiceBox(const std::vector<std::string> &options, int selected)
{
    int scale = PIXEL_SCALE;
    int boxWidth = 40 * scale;
    int itemHeight = 16 * scale + 8; // glyph height * scale + padding
    int boxHeight = static_cast<int>(options.size()) * itemHeight + 16;
    int boxX = WINDOW_WIDTH - boxWidth - 20;
    int boxY = WINDOW_HEIGHT - UI_PANEL_HEIGHT - boxHeight - 10;

    renderer.drawFilledRect(boxX, boxY, boxWidth, boxHeight, TDT4102::Color::white);
    renderer.drawRect(boxX, boxY, boxWidth, boxHeight,
                      TDT4102::Color::transparent, TDT4102::Color::black);

    for (int i = 0; i < static_cast<int>(options.size()); ++i)
    {
        int oy = boxY + 8 + i * itemHeight;
        if (i == selected)
            drawSelectionArrow(boxX + 2 * scale, oy + 4 * scale, scale);
        spriteFont.drawText(renderer, options[i], boxX + 6 * scale, oy, scale);
    }
}

// --- Battle intro transition (moved to BattleIntroMode) ---

// --- Helpers ---

void GameUI::drawTextBox(int x, int y, int width, int height, const std::string &text)
{
    renderer.drawFilledRect(x, y, width, height, TDT4102::Color::white);
    renderer.drawRect(x, y, width, height, TDT4102::Color::transparent, TDT4102::Color::black);
    spriteFont.drawText(renderer, text, x + 16, y + 14, PIXEL_SCALE);
}

void GameUI::drawTextBar(int panelY)
{
    int scale = PIXEL_SCALE;
    int barW = 252 * scale;
    int barH = 46 * scale;
    int barX = (WINDOW_WIDTH - barW) / 2;
    int barY = panelY + (UI_PANEL_HEIGHT - barH) / 2;
    renderer.drawSpriteRaw("ui_text_bar", barX, barY, barW, barH);
}

void GameUI::drawNarrowTextBar(int x, int y, int srcW, int scale)
{
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
    renderer.drawSpriteRegion("ui_text_bar",
                              0, 0, edgeW, srcH,
                              x, y, edgeW * scale, dstH);

    // Middle: tile from center of source bar
    int midSrcX = fullSrcW / 2 - middleSrcW / 2;
    renderer.drawSpriteRegion("ui_text_bar",
                              midSrcX, 0, middleSrcW, srcH,
                              x + edgeW * scale, y, middleSrcW * scale, dstH);

    // Right edge
    renderer.drawSpriteRegion("ui_text_bar",
                              fullSrcW - edgeW, 0, edgeW, srcH,
                              x + (edgeW + middleSrcW) * scale, y, edgeW * scale, dstH);
}

void GameUI::drawSelectionArrow(int x, int y, int scale)
{
    // Pixel-art right-pointing arrow (5 pixels tall at source scale)
    int s = scale;
    renderer.drawFilledRect(x, y + 0 * s, 1 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 1 * s, 2 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 2 * s, 3 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 3 * s, 2 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 4 * s, 1 * s, 1 * s, TDT4102::Color::black);
}

// Daemon info bars

void GameUI::drawOpponentInfoBar(const Daemon *opponentDaemon, int offsetX)
{
    const int scale = PIXEL_SCALE;

    // === Opponent info panel (top-left) ===
    int oppPanelW = 122 * scale; // 366
    int oppPanelH = 35 * scale;  // 105
    int oppPanelX = offsetX;
    int oppPanelY = 16;

    renderer.drawSpriteRaw("ui_opponent_info", oppPanelX, oppPanelY, oppPanelW, oppPanelH);

    if (opponentDaemon)
    {
        // Opponent name (drawn on top of the panel)
        int oppNameX = oppPanelX + 2 * scale;
        int oppNameY = oppPanelY + 9 * scale;
        spriteFont.drawText(renderer, opponentDaemon->getNickname(), oppNameX, oppNameY, scale);

        // Opponent level number - draw using battle numbers
        int oppLvlX = oppPanelX + 83 * scale;
        int oppLvlY = oppPanelY + 12 * scale;
        spriteFont.drawBattleNumber(renderer, opponentDaemon->getLevel(), oppLvlX, oppLvlY, scale);

        // Opponent HP bar
        int oppHPBarX = oppPanelX + 50 * scale;
        int oppHPBarY = oppPanelY + 24 * scale;
        int oppHPBarW = 48 * scale; // bar width in destination pixels
        drawSpriteHPBar(oppHPBarX, oppHPBarY, oppHPBarW,
                        opponentDisplayHP, opponentDaemon->getMaxHP(), scale);
    }
}
void GameUI::drawPlayerInfoBar(const Daemon *playerDaemon, int offsetX)
{
    const int scale = PIXEL_SCALE;
    int battleH = WINDOW_HEIGHT - UI_PANEL_HEIGHT;

    // === Player info panel (bottom-right) ===
    int plyPanelW = 128 * scale; // 384
    int plyPanelH = 47 * scale;  // 141
    int plyPanelX = WINDOW_WIDTH - plyPanelW + offsetX;
    int plyPanelY = battleH - plyPanelH - 10;

    renderer.drawSpriteRaw("ui_player_info", plyPanelX, plyPanelY, plyPanelW, plyPanelH);

    if (playerDaemon)
    {
        // Player name
        int plyNameX = plyPanelX + 18 * scale;
        int plyNameY = plyPanelY + 10 * scale;
        spriteFont.drawText(renderer, playerDaemon->getNickname(), plyNameX, plyNameY, scale);

        // Player level number
        int plyLvlX = plyPanelX + 105 * scale;
        int plyLvlY = plyPanelY + 12 * scale;
        spriteFont.drawBattleNumber(renderer, playerDaemon->getLevel(), plyLvlX, plyLvlY, scale);

        // Player HP bar
        int plyHPBarX = plyPanelX + 72 * scale;
        int plyHPBarY = plyPanelY + 25 * scale;
        int plyHPBarW = 48 * scale;
        drawSpriteHPBar(plyHPBarX, plyHPBarY, plyHPBarW,
                        playerDisplayHP, playerDaemon->getMaxHP(), scale);

        // Player HP numbers (current/max) using battle numbers
        int hpNumX = plyPanelX + 92 * scale;
        int hpNumY = plyPanelY + 32 * scale;
        int hpPad = 3 * scale;
        spriteFont.drawBattleNumber(renderer, playerDisplayHP, hpNumX - hpPad, hpNumY, scale, true);
        spriteFont.drawBattleNumber(renderer, playerDaemon->getMaxHP(), hpNumX + hpPad, hpNumY, scale);

        // Player EXP bar
        int expBarX = plyPanelX + 24 * scale;
        int expBarY = plyPanelY + 42 * scale;
        int expBarW = 96 * scale;
        int expNeeded = playerDaemon->getExpNeeded();
        drawSpriteEXPBar(expBarX, expBarY, expBarW, playerDisplayEXP, expNeeded, scale);
    }
}

void GameUI::drawOpponentDaemon(const Daemon *opponentDaemon, int offsetX)
{
    int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getOpponentBaseGeometry();

    if (opponentDaemon)
    {
        const std::string &opponentSpeciesName = opponentDaemon->getSpecies().name;
        loadDaemonSprite(opponentSpeciesName);
        std::string oppSpriteId = "daemon_" + opponentDaemon->getSpecies().name;
        TDT4102::Image &oppImg = renderer.getTexture(oppSpriteId);
        int oppSprW = oppImg.width * scale;
        int oppSprH = oppImg.height * scale;
        int oppSprX = baseX + baseW / 2 - oppSprW / 2 + offsetX;
        int oppSprY = baseY - oppSprH + baseH - 10 * scale;
        renderer.drawSpriteRaw(oppSpriteId, oppSprX, oppSprY, oppSprW, oppSprH);
    }
}
void GameUI::drawPlayerDaemon(const Daemon *playerDaemon, int offsetX)
{
    const int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getPlayerBaseGeometry();

    if (playerDaemon)
    {
        const std::string &playerSpeciesName = playerDaemon->getSpecies().name;
        loadDaemonSprite(playerSpeciesName);
        std::string plySpriteId = "daemon_" + playerDaemon->getSpecies().name + "_back";
        TDT4102::Image &plyImg = renderer.getTexture(plySpriteId);
        int plySprW = plyImg.width * scale;
        int plySprH = plyImg.height * scale;
        int plySprX = baseX + baseW / 2 - plySprW / 2 + offsetX;
        int plySprY = baseY - plySprH + baseH;
        renderer.drawSpriteRaw(plySpriteId, plySprX, plySprY, plySprW, plySprH);
    }
}

// --- Player back spritesheet (3x3 grid of 80x80, 5 frames used) ---
void GameUI::drawPlayerBackSprite(int x, int y, int dstW, int dstH, int frame)
{
    constexpr int FRAME_W = 80;
    constexpr int FRAME_H = 80;
    constexpr int COLS = 3;
    constexpr int TOTAL_FRAMES = 5;
    int f = frame % TOTAL_FRAMES;
    int srcX = (f % COLS) * FRAME_W;
    int srcY = (f / COLS) * FRAME_H;
    renderer.drawSpriteRegion("player_back", srcX, srcY, FRAME_W, FRAME_H,
                              x, y, dstW, dstH);
}

void GameUI::drawBattleBackground()
{
    int battleH = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, battleH, TDT4102::Color{200, 220, 200});
}

GameUI::BattleBaseGeometry GameUI::getPlayerBaseGeometry() const
{
    const int scale = PIXEL_SCALE;
    int battleH = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    int w = 256 * scale;
    int h = 32 * scale;
    return {-60 * scale, battleH - h, w, h};
}

GameUI::BattleBaseGeometry GameUI::getOpponentBaseGeometry() const
{
    const int scale = PIXEL_SCALE;
    int w = 128 * scale;
    int h = 64 * scale;
    return {WINDOW_WIDTH - w - 60, 120, w, h};
}

void GameUI::drawPlayerBase()
{
    auto [x, y, w, h] = getPlayerBaseGeometry();
    renderer.drawSpriteRaw("ui_player_base", x, y, w, h);
}

void GameUI::drawOpponentBase(int offsetX)
{
    auto [x, y, w, h] = getOpponentBaseGeometry();
    renderer.drawSpriteRaw("ui_opponent_base", x + offsetX, y, w, h);
}

void GameUI::drawOpponentTrainer(const NPC *opponent, int offsetX)
{
    const int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getOpponentBaseGeometry();

    // Draw a silhouette rectangle as trainer placeholder
    int trainerW = 40 * scale;
    int trainerH = 56 * scale;
    int trainerX = baseX + baseW / 2 - trainerW / 2 + offsetX;
    int trainerY = baseY + baseH - trainerH - 10 * scale;
    renderer.drawFilledRect(trainerX, trainerY, trainerW, trainerH,
                            TDT4102::Color{60, 60, 80});
    if (opponent)
    {
        spriteFont.drawText(renderer, opponent->getName(),
                            trainerX + 2 * scale, trainerY + trainerH / 2, scale - 1);
    }
}

void GameUI::drawPlayerBackOnBase(int offsetX, int frame)
{
    const int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getPlayerBaseGeometry();

    int backW = 80 * scale;
    int backH = 80 * scale;
    int backX = baseX + baseW / 2 - backW / 2 + offsetX;
    int backY = baseY - backH + baseH;
    drawPlayerBackSprite(backX, backY, backW, backH, frame);
}

void GameUI::drawPlayerSendOutPhase(const Daemon *playerDaemon, float t)
{
    // Player back sprite slides out to the left (first half)
    int playerSlideOut = static_cast<int>(-WINDOW_WIDTH * t);
    int throwFrame = battleIntroFrame / 8;
    if (t < 0.5f)
        drawPlayerBackOnBase(playerSlideOut, throwFrame);

    // Player Daemon slides in from off-screen left
    int daemonSlideIn = static_cast<int>(-WINDOW_WIDTH * (1.0f - t));
    drawPlayerDaemon(playerDaemon, daemonSlideIn);

    // Player info bar slides in from right
    int infoSlideIn = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
    drawPlayerInfoBar(playerDaemon, infoSlideIn);
}

// --- Sprite-based HP Bar ---
// Uses hp_bar.png (3x12): rows 0-2 green, 3-5 yellow, 6-8 red, 9-11 empty
void GameUI::drawSpriteHPBar(int x, int y, int width, int currentHP, int maxHP, int scale)
{
    float ratio = (maxHP > 0) ? static_cast<float>(currentHP) / static_cast<float>(maxHP) : 0.0f;
    int filledWidth = static_cast<int>(static_cast<float>(width) * ratio);

    // Choose color variant based on HP ratio
    // hp_bar.png rows: 0-2 = green, 3-5 = yellow, 6-8 = red, 9-11 = empty
    int emptySrcY = 9; // empty/gray variant (rows 9-11)
    int fillSrcY;
    if (ratio > 0.5f)
        fillSrcY = 0; // green (rows 0-2)
    else if (ratio > 0.2f)
        fillSrcY = 3; // yellow (rows 3-5)
    else
        fillSrcY = 6; // red (rows 6-8)

    int barH = 3 * scale; // 3 source pixels tall, scaled

    // Draw empty background across full width
    // We tile the 3px wide source horizontally
    for (int dx = 0; dx < width; dx += scale)
    {
        int tileW = std::min(scale, width - dx);
        renderer.drawSpriteRegion("ui_hp_bar",
                                  0, emptySrcY, 1, 3,
                                  x + dx, y, tileW, barH);
    }

    // Draw filled portion
    if (filledWidth > 0)
    {
        for (int dx = 0; dx < filledWidth; dx += scale)
        {
            int tileW = std::min(scale, filledWidth - dx);
            renderer.drawSpriteRegion("ui_hp_bar",
                                      0, fillSrcY, 1, 3,
                                      x + dx, y, tileW, barH);
        }
    }
}

// --- Sprite-based EXP Bar ---
// Uses exp_bar.png (3x4): rows 0-1 empty, rows 2-3 filled
void GameUI::drawSpriteEXPBar(int x, int y, int width, int currentEXP, int maxEXP, int scale)
{
    int filledWidth = 0;
    if (maxEXP > 0)
        filledWidth = std::min(width, currentEXP * width / maxEXP);

    int barH = 2 * scale; // 2 source pixels tall, scaled

    // Draw empty background
    for (int dx = 0; dx < width; dx += scale)
    {
        int tileW = std::min(scale, width - dx);
        renderer.drawSpriteRegion("ui_exp_bar",
                                  0, 0, 1, 2,
                                  x + dx, y, tileW, barH);
    }

    // Draw filled portion
    if (filledWidth > 0)
    {
        for (int dx = 0; dx < filledWidth; dx += scale)
        {
            int tileW = std::min(scale, filledWidth - dx);
            renderer.drawSpriteRegion("ui_exp_bar",
                                      0, 2, 1, 2,
                                      x + dx, y, tileW, barH);
        }
    }
}
