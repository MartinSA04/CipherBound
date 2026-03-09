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

    renderer.loadTexture("ui_player_info", "assets/sprites/ui/player_creature_info.png");
    renderer.loadTexture("ui_opponent_info", "assets/sprites/ui/opponent_creature_info.png");
    renderer.loadTexture("ui_hp_bar", "assets/sprites/ui/hp_bar.png");
    renderer.loadTexture("ui_exp_bar", "assets/sprites/ui/exp_bar.png");
    renderer.loadTexture("ui_player_base", "assets/sprites/ui/player_battle_base.png");
    renderer.loadTexture("ui_opponent_base", "assets/sprites/ui/opponent_battle_base.png");
    renderer.loadTexture("player_back", "assets/sprites/player/player_back.png");

    battleAssetsLoaded = true;
}

void GameUI::loadCreatureSprite(const std::string &speciesName)
{
    std::string frontId = "creature_" + speciesName;
    std::string backId = "creature_" + speciesName + "_back";
    std::string frontPath = "assets/sprites/creatures/" + speciesName + ".png";
    std::string backPath = "assets/sprites/creatures/" + speciesName + "_back.png";

    if (!renderer.hasTexture(frontId))
        renderer.loadTexture(frontId, frontPath);
    if (!renderer.hasTexture(backId))
        renderer.loadTexture(backId, backPath);
}

void GameUI::drawBattleScene(const Creature *playerCreature,
                             const Creature *opponentCreature)
{
    // Ensure battle assets are loaded
    loadBattleAssets();

    // Background
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT - UI_PANEL_HEIGHT,
                            TDT4102::Color{200, 220, 200});

    // Bases first
    drawOpponentBase();
    drawPlayerBase();

    // Sprites on top
    drawOpponentCreature(opponentCreature);
    drawPlayerCreature(playerCreature);
    drawOpponentInfoBar(opponentCreature);
    drawPlayerInfoBar(playerCreature);
}

void GameUI::drawBattleIntroScene(const Creature *playerCreature, const Creature *opponentCreature)
{
    // Wild battle intro — phases:
    //   Phase 0: Wild creature slides in from the right, player back sprite visible
    //   Phase 1: Player back sprite slides out left, player creature slides in from left

    loadBattleAssets();

    float t = static_cast<float>(battleIntroFrame) / static_cast<float>(BATTLE_INTRO_SCENE_DURATION);
    if (t > 1.0f)
        t = 1.0f;

    drawBattleBackground();

    if (battleIntroPhase == 0)
    {
        // Bases
        drawOpponentBase();
        drawPlayerBase();
        // Sprites on top
        int slideOffset = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        drawOpponentCreature(opponentCreature, slideOffset);
        drawPlayerBackOnBase();
    }
    else if (battleIntroPhase == 1)
    {
        // Bases
        drawOpponentBase();
        drawPlayerBase();
        // Sprites on top
        drawOpponentCreature(opponentCreature);
        drawOpponentInfoBar(opponentCreature);
        drawPlayerSendOutPhase(playerCreature, t);
    }
}

void GameUI::drawBattleIntroScene(const Creature *playerCreature, const std::shared_ptr<NPC> opponent, const Creature *opponentCreature)
{
    // Trainer battle intro — phases:
    //   Phase 0: Trainer NPC appears from the right, player back sprite visible
    //   Phase 1: Trainer slides out to right, opponent creature slides in from right
    //   Phase 2: Player back sprite slides out left, player creature slides in from left

    loadBattleAssets();

    float t = static_cast<float>(battleIntroFrame) / static_cast<float>(BATTLE_INTRO_SCENE_DURATION);
    if (t > 1.0f)
        t = 1.0f;

    drawBattleBackground();

    if (battleIntroPhase == 0)
    {
        // Bases
        drawOpponentBase();
        drawPlayerBase();
        // Sprites on top
        int slideOffset = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        drawOpponentTrainer(opponent.get(), slideOffset);
        drawPlayerBackOnBase();
    }
    else if (battleIntroPhase == 1)
    {
        // Bases
        drawOpponentBase();
        drawPlayerBase();
        // Sprites on top
        int trainerOut = static_cast<int>(WINDOW_WIDTH * t);
        drawOpponentTrainer(opponent.get(), trainerOut);

        int creatureIn = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        drawOpponentCreature(opponentCreature, creatureIn);

        drawPlayerBackOnBase();
    }
    else if (battleIntroPhase == 2)
    {
        // Bases
        drawOpponentBase();
        drawPlayerBase();
        // Sprites on top
        drawOpponentCreature(opponentCreature);
        drawOpponentInfoBar(opponentCreature);
        drawPlayerSendOutPhase(playerCreature, t);
    }
}

void GameUI::drawBattleMenu(const std::vector<std::string> &options, int selected)
{
    int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;

    // Draw text bar background
    drawTextBar(panelY);

    // Options in a 2x2 grid on the right side
    int menuX = WINDOW_WIDTH / 2;
    int menuWidth = WINDOW_WIDTH / 2;
    int optionHeight = UI_PANEL_HEIGHT / 2 - 10;
    int scale = PIXEL_SCALE;

    for (int i = 0; i < static_cast<int>(options.size()) && i < 4; ++i)
    {
        int col = i % 2;
        int row = i / 2;
        int ox = menuX + col * (menuWidth / 2) + 20;
        int oy = panelY + row * optionHeight + 23;

        if (i == selected)
        {
            drawSelectionArrow(ox - 16, oy + 4 * scale, scale);
        }
        spriteFont.drawText(renderer, options[i], ox, oy, scale);
    }
}

void GameUI::drawMoveSelect(const Creature &creature, const Pokedex &pokedex, int selected)
{
    int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    int scale = PIXEL_SCALE;

    // Draw full text bar background for the move names area
    drawTextBar(panelY);

    // --- Move names in a 2x2 grid on the left side ---
    int textBarW = 252 * scale;
    int textBarX = (WINDOW_WIDTH - textBarW) / 2;
    int gridX = textBarX + 8 * scale;
    int gridY = panelY + (UI_PANEL_HEIGHT - 46 * scale) / 2 + 5 * scale;
    int colW = (textBarW / 2) - 12 * scale;
    int rowH = 18 * scale;

    const auto &moves = creature.getMoves();

    // Find the selected move data for the info panel
    const MoveData *selectedMove = nullptr;

    for (int i = 0; i < 4; ++i)
    {
        int col = i % 2;
        int row = i / 2;
        int ox = gridX + col * colW;
        int oy = gridY + row * rowH;

        if (moves[i].moveId < 0)
        {
            spriteFont.drawText(renderer, "---", ox + 6 * scale, oy, scale);
            continue;
        }

        const MoveData &moveData = pokedex.getMove(moves[i].moveId);

        if (i == selected)
        {
            selectedMove = &moveData;
            drawSelectionArrow(ox + scale, oy + 4 * scale, scale);
        }

        spriteFont.drawText(renderer, moveData.name, ox + 6 * scale, oy, scale);
    }

    // --- Info box on the right using a narrow text bar ---
    // Draw a narrower text_bar to the right of the main bar
    int infoSrcW = 100; // source pixels wide
    int infoH = 46 * scale;
    int infoX = 0;
    int infoY = panelY - infoH;
    drawNarrowTextBar(infoX, infoY, infoSrcW, scale);

    // Draw PP and type info for the selected move
    if (selectedMove && selected >= 0 && selected < 4 && moves[selected].moveId >= 0)
    {
        int labelX = infoX + 10 * scale;
        int labelY1 = infoY + 5 * scale;
        int labelY2 = infoY + 24 * scale;

        // PP line
        std::string ppText = "PP " + std::to_string(moves[selected].currentPP) + "-" +
                             std::to_string(moves[selected].maxPP);
        spriteFont.drawText(renderer, ppText, labelX, labelY1, scale);

        // Type line
        std::string typeName = elementTypeName(selectedMove->type);
        // Capitalize first letter
        if (!typeName.empty())
            typeName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(typeName[0])));
        spriteFont.drawText(renderer, typeName, labelX, labelY2, scale);
    }
}

// --- Menus ---

void GameUI::drawMainMenu(int selected)
{
    // Semi-transparent overlay on right side
    static const std::vector<std::string> menuItems = {"Pokemon", "Bag", "Save", "Exit"};
    int scale = PIXEL_SCALE;
    int itemHeight = 18 * scale + 6; // glyph height * scale + padding
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
            drawSelectionArrow(menuX + 2 * scale, oy + 4 * scale, scale);
        }

        spriteFont.drawText(renderer, menuItems[i], menuX + 6 * scale, oy, scale);
    }
}

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

        const Creature &c = party[i];

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

void GameUI::drawPCBoxScreen(const Player &player, int selected, bool viewingParty)
{
    int scale = PIXEL_SCALE;
    int halfW = WINDOW_WIDTH / 2;

    // Background
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{50, 60, 90});

    // Title: "Box N" with left/right arrows
    std::string boxTitle = "< Box " + std::to_string(player.getCurrentBox() + 1) + " >";
    int titleX = halfW - static_cast<int>(boxTitle.size()) * 4 * scale;
    spriteFont.drawText(renderer, boxTitle, titleX, 8, scale);

    // --- Left panel: Party ---
    int panelY = 12 * scale;
    spriteFont.drawText(renderer, "PARTY", 28, panelY, scale);
    panelY += 12 * scale;

    int slotH = 12 * scale;
    int slotGap = 4;
    const auto &party = player.getParty();

    for (int i = 0; i < 6; ++i)
    {
        int sy = panelY + i * (slotH + slotGap);
        bool hasCreature = i < static_cast<int>(party.size());

        TDT4102::Color bg = TDT4102::Color{70, 80, 110};
        if (viewingParty && i == selected)
            bg = TDT4102::Color{140, 160, 220};

        renderer.drawFilledRect(10, sy, halfW - 20, slotH, bg);
        renderer.drawRect(10, sy, halfW - 20, slotH,
                          TDT4102::Color::transparent, TDT4102::Color{100, 110, 140});

        if (hasCreature)
        {
            const Creature &c = party[i];
            if (viewingParty && i == selected)
                drawSelectionArrow(16, sy + 2 * scale, scale);

            spriteFont.drawText(renderer, c.getNickname(), 16 + 6 * scale, sy + 2, scale);
            spriteFont.drawText(renderer, "Lv" + std::to_string(c.getLevel()),
                                halfW - 80 - 12 * scale, sy + 2, scale);
        }
        else
        {
            spriteFont.drawText(renderer, "---", 16 + 6 * scale, sy + 2, scale);
        }
    }

    // --- Right panel: Box contents ---
    int boxPanelX = halfW + 10;
    spriteFont.drawText(renderer, "BOX", boxPanelX + 8, 12 * scale, scale);
    int boxPanelY = 16 * scale + 8 * scale;

    const auto &box = player.getBox(player.getCurrentBox());
    int boxCount = static_cast<int>(box.size());

    // Show up to 30 slots, but only draw visible ones (scrollable if needed)
    int maxVisible = 10;
    int scrollOffset = 0;
    if (!viewingParty && selected >= maxVisible)
        scrollOffset = selected - maxVisible + 1;

    for (int i = 0; i < maxVisible && (i + scrollOffset) < Player::BOX_SIZE; ++i)
    {
        int idx = i + scrollOffset;
        int sy = boxPanelY + i * (slotH + slotGap);
        bool hasCreature = idx < boxCount;

        TDT4102::Color bg = TDT4102::Color{70, 80, 110};
        if (!viewingParty && idx == selected)
            bg = TDT4102::Color{140, 160, 220};

        renderer.drawFilledRect(boxPanelX, sy, halfW - 20, slotH, bg);
        renderer.drawRect(boxPanelX, sy, halfW - 20, slotH,
                          TDT4102::Color::transparent, TDT4102::Color{100, 110, 140});

        if (hasCreature)
        {
            const Creature &c = box[idx];
            if (!viewingParty && idx == selected)
                drawSelectionArrow(boxPanelX + 6, sy + 2 * scale, scale);

            spriteFont.drawText(renderer, c.getNickname(), boxPanelX + 6 + 6 * scale, sy + 2, scale);
            spriteFont.drawText(renderer, "Lv" + std::to_string(c.getLevel()),
                                WINDOW_WIDTH - 80 - 12 * scale, sy + 2, scale);
        }
        else
        {
            spriteFont.drawText(renderer, "---", boxPanelX + 6 + 6 * scale, sy + 2, scale);
        }
    }

    // Status bar at bottom
    int statusY = WINDOW_HEIGHT - 10 * scale;
    if (viewingParty)
        spriteFont.drawText(renderer, "A:Deposit  LR:Switch Box  B:Exit", 20, statusY, scale - 1);
    else
        spriteFont.drawText(renderer, "A:Withdraw  LR:Switch Box  B:Exit", 20, statusY, scale - 1);
}

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

void GameUI::drawTitleScreen(const std::vector<SaveManager::SlotInfo> &slots, int selected)
{
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{20, 20, 60});

    // Title
    spriteFont.drawText(renderer, "POCKET CREATURES",
                        WINDOW_WIDTH / 2 - 8 * 16 * PIXEL_SCALE / 2, 40, PIXEL_SCALE, 1);

    // Subtitle
    renderer.drawText("Select a save slot", WINDOW_WIDTH / 2 - 70, 90,
                      TDT4102::Color::light_gray, 14);

    // Draw save slots
    int slotBoxW = 600;
    int slotBoxH = 80;
    int startY = 130;
    int spacing = 10;
    int startX = (WINDOW_WIDTH - slotBoxW) / 2;

    for (size_t i = 0; i < slots.size(); ++i)
    {
        int y = startY + static_cast<int>(i) * (slotBoxH + spacing);
        bool isSelected = (static_cast<int>(i) == selected);

        // Slot background
        TDT4102::Color bgColor = isSelected
                                     ? TDT4102::Color{60, 60, 120}
                                     : TDT4102::Color{35, 35, 80};
        renderer.drawFilledRect(startX, y, slotBoxW, slotBoxH, bgColor);

        // Border
        TDT4102::Color borderColor = isSelected
                                         ? TDT4102::Color{200, 200, 255}
                                         : TDT4102::Color{80, 80, 130};
        renderer.drawRect(startX, y, slotBoxW, slotBoxH, borderColor);

        // Selection arrow
        if (isSelected)
            drawSelectionArrow(startX + 8, y + slotBoxH / 2 - PIXEL_SCALE * 3, PIXEL_SCALE);

        int textX = startX + 40;
        int textY = y + 10;

        std::string slotLabel = "Slot " + std::to_string(i + 1);
        renderer.drawText(slotLabel, textX, textY, TDT4102::Color::white, 18);

        if (slots[i].exists)
        {
            std::string info = slots[i].playerName;
            info += "   Party: " + std::to_string(slots[i].partySize);
            info += "   Badges: " + std::to_string(slots[i].badgeCount);
            if (!slots[i].mapId.empty())
                info += "   Map: " + slots[i].mapId;
            renderer.drawText(info, textX, textY + 30, TDT4102::Color::light_gray, 14);
        }
        else
        {
            renderer.drawText("New Game", textX, textY + 30, TDT4102::Color{140, 140, 180}, 14);
        }
    }

    // Controls hint
    renderer.drawText("Arrow keys to select, Z or Enter to confirm",
                      WINDOW_WIDTH / 2 - 160, WINDOW_HEIGHT - 40,
                      TDT4102::Color{100, 100, 150}, 12);
}

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

void GameUI::drawFade(float alpha)
{
    if (alpha <= 0.0f)
        return;
    unsigned char a = static_cast<unsigned char>(alpha * 255.0f);
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{0, 0, 0, a});
}

void GameUI::drawBattleIntro()
{
    // Battle intro transition phases (Pokemon-style):
    //   Phase 1 (0-11):    3 quick white flashes over the overworld
    //   Phase 2 (12-49):   Horizontal bars sweep across the screen
    //   Phase 3 (50-89):   Fade from bars to solid black, then hold

    constexpr int FLASH_END = 12;
    constexpr int BARS_END = 50;
    const int frame = battleIntroFrame;

    if (frame < FLASH_END)
    {
        // Phase 1: Quick white flashes (3 flashes, 4 frames each)
        int flashCycle = frame % 4;
        if (flashCycle < 2)
        {
            // Flash white
            unsigned char a = (flashCycle == 0) ? 200 : 120;
            renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                                    TDT4102::Color{255, 255, 255, a});
        }
    }
    else if (frame < BARS_END)
    {
        // Phase 2: Horizontal bars sweep in from alternating sides
        int barPhase = frame - FLASH_END;
        int totalBarFrames = BARS_END - FLASH_END; // 38 frames
        int numBars = 8;
        int barHeight = WINDOW_HEIGHT / numBars;

        for (int i = 0; i < numBars; ++i)
        {
            // Stagger: each bar starts a few frames after the previous
            int barDelay = i * (totalBarFrames / (numBars + 2));
            int barProgress = barPhase - barDelay;
            if (barProgress < 0)
                continue;

            // Progress 0..1 for this bar
            float t = std::min(1.0f, static_cast<float>(barProgress) / static_cast<float>(totalBarFrames - barDelay));
            int barWidth = static_cast<int>(t * WINDOW_WIDTH);

            int barY = i * barHeight;
            // Alternate direction: even bars from left, odd from right
            if (i % 2 == 0)
            {
                renderer.drawFilledRect(0, barY, barWidth, barHeight, TDT4102::Color::black);
            }
            else
            {
                renderer.drawFilledRect(WINDOW_WIDTH - barWidth, barY, barWidth, barHeight, TDT4102::Color::black);
            }
        }
    }
    else
    {
        // Phase 3: Solid black (hold until transition completes)
        renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color::black);
    }
}

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

// creature info bars

void GameUI::drawOpponentInfoBar(const Creature *opponentCreature, int offsetX)
{
    const int scale = PIXEL_SCALE;

    // === Opponent info panel (top-left) ===
    int oppPanelW = 122 * scale; // 366
    int oppPanelH = 35 * scale;  // 105
    int oppPanelX = offsetX;
    int oppPanelY = 16;

    renderer.drawSpriteRaw("ui_opponent_info", oppPanelX, oppPanelY, oppPanelW, oppPanelH);

    if (opponentCreature)
    {
        // Opponent name (drawn on top of the panel)
        int oppNameX = oppPanelX + 2 * scale;
        int oppNameY = oppPanelY + 9 * scale;
        spriteFont.drawText(renderer, opponentCreature->getNickname(), oppNameX, oppNameY, scale);

        // Opponent level number - draw using battle numbers
        int oppLvlX = oppPanelX + 83 * scale;
        int oppLvlY = oppPanelY + 12 * scale;
        spriteFont.drawBattleNumber(renderer, opponentCreature->getLevel(), oppLvlX, oppLvlY, scale);

        // Opponent HP bar
        int oppHPBarX = oppPanelX + 50 * scale;
        int oppHPBarY = oppPanelY + 24 * scale;
        int oppHPBarW = 48 * scale; // bar width in destination pixels
        drawSpriteHPBar(oppHPBarX, oppHPBarY, oppHPBarW,
                        opponentDisplayHP, opponentCreature->getMaxHP(), scale);
    }
}
void GameUI::drawPlayerInfoBar(const Creature *playerCreature, int offsetX)
{
    const int scale = PIXEL_SCALE;
    int battleH = WINDOW_HEIGHT - UI_PANEL_HEIGHT;

    // === Player info panel (bottom-right) ===
    int plyPanelW = 128 * scale; // 384
    int plyPanelH = 47 * scale;  // 141
    int plyPanelX = WINDOW_WIDTH - plyPanelW + offsetX;
    int plyPanelY = battleH - plyPanelH - 10;

    renderer.drawSpriteRaw("ui_player_info", plyPanelX, plyPanelY, plyPanelW, plyPanelH);

    if (playerCreature)
    {
        // Player name
        int plyNameX = plyPanelX + 18 * scale;
        int plyNameY = plyPanelY + 10 * scale;
        spriteFont.drawText(renderer, playerCreature->getNickname(), plyNameX, plyNameY, scale);

        // Player level number
        int plyLvlX = plyPanelX + 105 * scale;
        int plyLvlY = plyPanelY + 12 * scale;
        spriteFont.drawBattleNumber(renderer, playerCreature->getLevel(), plyLvlX, plyLvlY, scale);

        // Player HP bar
        int plyHPBarX = plyPanelX + 72 * scale;
        int plyHPBarY = plyPanelY + 25 * scale;
        int plyHPBarW = 48 * scale;
        drawSpriteHPBar(plyHPBarX, plyHPBarY, plyHPBarW,
                        playerDisplayHP, playerCreature->getMaxHP(), scale);

        // Player HP numbers (current/max) using battle numbers
        int hpNumX = plyPanelX + 92 * scale;
        int hpNumY = plyPanelY + 32 * scale;
        int hpPad = 3 * scale;
        spriteFont.drawBattleNumber(renderer, playerDisplayHP, hpNumX - hpPad, hpNumY, scale, true);
        spriteFont.drawBattleNumber(renderer, playerCreature->getMaxHP(), hpNumX + hpPad, hpNumY, scale);

        // Player EXP bar
        int expBarX = plyPanelX + 24 * scale;
        int expBarY = plyPanelY + 42 * scale;
        int expBarW = 96 * scale;
        int expNeeded = playerCreature->getExpNeeded();
        drawSpriteEXPBar(expBarX, expBarY, expBarW, playerDisplayEXP, expNeeded, scale);
    }
}

void GameUI::drawOpponentCreature(const Creature *opponentCreature, int offsetX)
{
    int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getOpponentBaseGeometry();

    if (opponentCreature)
    {
        const std::string &opponentSpeciesName = opponentCreature->getSpecies().name;
        loadCreatureSprite(opponentSpeciesName);
        std::string oppSpriteId = "creature_" + opponentCreature->getSpecies().name;
        TDT4102::Image &oppImg = renderer.getTexture(oppSpriteId);
        int oppSprW = oppImg.width * scale;
        int oppSprH = oppImg.height * scale;
        int oppSprX = baseX + baseW / 2 - oppSprW / 2 + offsetX;
        int oppSprY = baseY - oppSprH + baseH - 10 * scale;
        renderer.drawSpriteRaw(oppSpriteId, oppSprX, oppSprY, oppSprW, oppSprH);
    }
}
void GameUI::drawPlayerCreature(const Creature *playerCreature, int offsetX)
{
    const int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getPlayerBaseGeometry();

    if (playerCreature)
    {
        const std::string &playerSpeciesName = playerCreature->getSpecies().name;
        loadCreatureSprite(playerSpeciesName);
        std::string plySpriteId = "creature_" + playerCreature->getSpecies().name + "_back";
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

void GameUI::drawPlayerSendOutPhase(const Creature *playerCreature, float t)
{
    // Player back sprite slides out to the left (first half)
    int playerSlideOut = static_cast<int>(-WINDOW_WIDTH * t);
    int throwFrame = battleIntroFrame / 8;
    if (t < 0.5f)
        drawPlayerBackOnBase(playerSlideOut, throwFrame);

    // Player creature slides in from off-screen left
    int creatureSlideIn = static_cast<int>(-WINDOW_WIDTH * (1.0f - t));
    drawPlayerCreature(playerCreature, creatureSlideIn);

    // Player info bar slides in from right
    int infoSlideIn = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
    drawPlayerInfoBar(playerCreature, infoSlideIn);
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
