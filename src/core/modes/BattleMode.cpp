#include "BattleMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../ui/Renderer.h"
#include "../../ui/SpriteFont.h"
#include "../../battle/Battle.h"
#include "../../world/World.h"
#include "../../world/Player.h"
#include "../../world/NPC.h"
#include "../../data/Pokedex.h"
#include "../../data/Species.h"
#include "../../audio/MusicManager.h"
#include <algorithm>
#include <cctype>

void BattleMode::setTrainerNPCId(const std::string &id)
{
    currentTrainerNPCId = id;
}

void BattleMode::setTrainer(std::shared_ptr<NPC> trainer)
{
    battleTrainer = std::move(trainer);
}

const std::string &BattleMode::getTrainerNPCId() const
{
    return currentTrainerNPCId;
}

void BattleMode::updateBattleIntroAnim(GameContext &ctx)
{
    ctx.ui.battleIntroFrame++;
    if (ctx.ui.battleIntroFrame >= GameUI::BATTLE_INTRO_SCENE_DURATION)
    {
        ctx.currentBattle->finishIntroAnimation();
        ctx.ui.battleIntroFrame = 0;
    }
}

void BattleMode::update(GameContext &ctx, InputManager &input)
{
    if (!ctx.currentBattle)
        return;

    BattleState bs = ctx.currentBattle->getState();
    switch (bs)
    {
    case BattleState::intro:
        updateBattleIntroAnim(ctx);
        return;

    case BattleState::victory:
    case BattleState::defeat:
    case BattleState::fled:
    case BattleState::captured:
        if (bs == BattleState::victory || bs == BattleState::captured)
        {
            MusicTrack victoryTrack = (battleTrainer && battleTrainer->isTrainerType())
                                          ? MusicTrack::trainerVictory
                                          : MusicTrack::wildVictory;
            ctx.music.play(victoryTrack, ctx.ui.getRenderer().getWindow());
        }
        else if (bs == BattleState::defeat)
        {
            ctx.music.stop();
        }

        if (ctx.ui.updateTypewriter(input.isConfirmPressed()))
            ctx.pushRequest(ModeRequest::endBattle());
        return;

    case BattleState::showingMessages:
        if (!ctx.currentBattle->isIntroComplete())
            ctx.ui.battleIntroFrame++;

        if (ctx.ui.updateTypewriter(input.isConfirmPressed()))
        {
            ctx.currentBattle->advanceMessage();
            if (ctx.currentBattle->getState() == BattleState::intro)
            {
                ctx.ui.battleIntroFrame = 0;
                ctx.ui.battleIntroPhase = ctx.currentBattle->getIntroPhase();
            }
        }
        return;

    case BattleState::animatingHP:
    {
        bool done = ctx.ui.tickHPAnimation(
            ctx.currentBattle->getPlayerDaemon().getCurrentHP(),
            ctx.currentBattle->getOpponentDaemon().getCurrentHP(),
            ctx.currentBattle->getPlayerDaemon().getMaxHP(),
            ctx.currentBattle->getOpponentDaemon().getMaxHP());
        if (done)
            ctx.currentBattle->finishHPAnimation();
        return;
    }

    case BattleState::animatingEXP:
    {
        BattleState bps = ctx.currentBattle->getPendingState();
        if (bps == BattleState::victory || bps == BattleState::captured)
        {
            MusicTrack victoryTrack = (battleTrainer && battleTrainer->isTrainerType())
                                          ? MusicTrack::trainerVictory
                                          : MusicTrack::wildVictory;
            ctx.music.play(victoryTrack, ctx.ui.getRenderer().getWindow());
        }

        Daemon &daemon = ctx.currentBattle->getPlayerDaemon();
        EXPTickResult result = ctx.ui.tickEXPAnimation(daemon.getExp(), daemon.getExpNeeded());

        if (result == EXPTickResult::filledBar && daemon.checkLevelUp())
        {
            ctx.ui.playerDisplayEXP = 0;
            ctx.currentBattle->addLevelUpMessage(
                daemon.getNickname() + " leveled up to Lv" + std::to_string(daemon.getLevel()) + "!");
            ctx.ui.playerDisplayHP = daemon.getCurrentHP();
        }
        else if (result == EXPTickResult::reachedTarget)
        {
            ctx.currentBattle->finishEXPAnimation();
        }
        return;
    }

    case BattleState::opponentTurn:
        ctx.currentBattle->executeOpponentTurn();
        return;

    case BattleState::choosingAction:
        ctx.ui.navigate2x2(menuSelected);

        if (input.isConfirmPressed())
        {
            BattleAction actions[] = {
                BattleAction::fight,
                BattleAction::item,
                BattleAction::switchDaemon,
                BattleAction::flee};
            ctx.currentBattle->chooseAction(actions[menuSelected]);
            moveSelected = 0;
            partySelected = 0;
            bagSelected = 0;
        }
        return;

    case BattleState::choosingMove:
        ctx.ui.navigate2x2(moveSelected);

        if (input.isConfirmPressed())
            ctx.currentBattle->chooseMove(moveSelected);
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;

    case BattleState::choosingSwitch:
        ctx.ui.navigateVertical(partySelected, ctx.world.getPlayer().partySize());

        if (input.isConfirmPressed())
            ctx.currentBattle->chooseSwitchTarget(partySelected);
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;

    case BattleState::choosingItem:
    {
        int itemCount = static_cast<int>(ctx.world.getPlayer().getInventory().size());
        if (itemCount == 0)
        {
            if (input.isCancelPressed())
                ctx.currentBattle->goBack();
            return;
        }

        ctx.ui.navigateVertical(bagSelected, itemCount);

        if (input.isConfirmPressed())
        {
            if (bagSelected >= 0 && bagSelected < itemCount)
            {
                const auto &inv = ctx.world.getPlayer().getInventory();
                ctx.currentBattle->chooseItem(inv[bagSelected].itemId);
                int newSize = static_cast<int>(ctx.world.getPlayer().getInventory().size());
                if (bagSelected >= newSize)
                    bagSelected = std::max(0, newSize - 1);
            }
        }
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;
    }
    }
}

void BattleMode::drawBattleScene(GameContext &ctx)
{
    GameUI &ui = ctx.ui;
    ui.loadBattleAssets();

    Renderer &renderer = ui.getRenderer();
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT - UI_PANEL_HEIGHT,
                            TDT4102::Color{200, 220, 200});

    ui.drawOpponentBase();
    ui.drawPlayerBase();

    const Daemon *playerDaemon = &ctx.currentBattle->getPlayerDaemon();
    const Daemon *opponentDaemon = &ctx.currentBattle->getOpponentDaemon();

    ui.drawOpponentDaemon(opponentDaemon);
    ui.drawPlayerDaemon(playerDaemon);
    ui.drawOpponentInfoBar(opponentDaemon);
    ui.drawPlayerInfoBar(playerDaemon);
}

void BattleMode::drawBattleIntroSceneWild(GameContext &ctx)
{
    GameUI &ui = ctx.ui;
    ui.loadBattleAssets();

    float t = static_cast<float>(ui.battleIntroFrame) / static_cast<float>(GameUI::BATTLE_INTRO_SCENE_DURATION);
    if (t > 1.0f)
        t = 1.0f;

    ui.drawBattleBackground();

    const Daemon *playerDaemon = &ctx.currentBattle->getPlayerDaemon();
    const Daemon *opponentDaemon = &ctx.currentBattle->getOpponentDaemon();

    if (ui.battleIntroPhase == 0)
    {
        ui.drawOpponentBase();
        ui.drawPlayerBase();
        int slideOffset = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        ui.drawOpponentDaemon(opponentDaemon, slideOffset);
        ui.drawPlayerBackOnBase();
    }
    else if (ui.battleIntroPhase == 1)
    {
        ui.drawOpponentBase();
        ui.drawPlayerBase();
        ui.drawOpponentDaemon(opponentDaemon);
        ui.drawOpponentInfoBar(opponentDaemon);
        ui.drawPlayerSendOutPhase(playerDaemon, t);
    }
}

void BattleMode::drawBattleIntroSceneTrainer(GameContext &ctx)
{
    GameUI &ui = ctx.ui;
    ui.loadBattleAssets();

    float t = static_cast<float>(ui.battleIntroFrame) / static_cast<float>(GameUI::BATTLE_INTRO_SCENE_DURATION);
    if (t > 1.0f)
        t = 1.0f;

    ui.drawBattleBackground();

    const Daemon *playerDaemon = &ctx.currentBattle->getPlayerDaemon();
    const Daemon *opponentDaemon = &ctx.currentBattle->getOpponentDaemon();
    NPC *opponent = ctx.currentBattle->getOpponent().get();

    if (ui.battleIntroPhase == 0)
    {
        ui.drawOpponentBase();
        ui.drawPlayerBase();
        int slideOffset = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        ui.drawOpponentTrainer(opponent, slideOffset);
        ui.drawPlayerBackOnBase();
    }
    else if (ui.battleIntroPhase == 1)
    {
        ui.drawOpponentBase();
        ui.drawPlayerBase();
        int trainerOut = static_cast<int>(WINDOW_WIDTH * t);
        ui.drawOpponentTrainer(opponent, trainerOut);
        int daemonIn = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        ui.drawOpponentDaemon(opponentDaemon, daemonIn);
        ui.drawPlayerBackOnBase();
    }
    else if (ui.battleIntroPhase == 2)
    {
        ui.drawOpponentBase();
        ui.drawPlayerBase();
        ui.drawOpponentDaemon(opponentDaemon);
        ui.drawOpponentInfoBar(opponentDaemon);
        ui.drawPlayerSendOutPhase(playerDaemon, t);
    }
}

void BattleMode::drawBattleMenu(GameContext &ctx)
{
    Renderer &renderer = ctx.ui.getRenderer();
    SpriteFont &spriteFont = ctx.ui.getSpriteFont();

    static const std::vector<std::string> options = {"Fight", "Bag", "Daemons", "Run"};

    int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    ctx.ui.drawTextBar(panelY);

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

        if (i == menuSelected)
        {
            ctx.ui.drawSelectionArrow(ox - 16, oy + 4 * scale, scale);
        }
        spriteFont.drawText(renderer, options[i], ox, oy, scale);
    }
}

void BattleMode::drawMoveSelectScreen(GameContext &ctx)
{
    Renderer &renderer = ctx.ui.getRenderer();
    SpriteFont &spriteFont = ctx.ui.getSpriteFont();
    const Daemon &daemon = ctx.currentBattle->getPlayerDaemon();

    int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    int scale = PIXEL_SCALE;

    ctx.ui.drawTextBar(panelY);

    int textBarW = 252 * scale;
    int textBarX = (WINDOW_WIDTH - textBarW) / 2;
    int gridX = textBarX + 8 * scale;
    int gridY = panelY + (UI_PANEL_HEIGHT - 46 * scale) / 2 + 5 * scale;
    int colW = (textBarW / 2) - 12 * scale;
    int rowH = 18 * scale;

    const auto &moves = daemon.getMoves();

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

        const MoveData &moveData = ctx.pokedex.getMove(moves[i].moveId);

        if (i == moveSelected)
        {
            selectedMove = &moveData;
            ctx.ui.drawSelectionArrow(ox + scale, oy + 4 * scale, scale);
        }

        spriteFont.drawText(renderer, moveData.name, ox + 6 * scale, oy, scale);
    }

    // Info box
    int infoSrcW = 100;
    int infoH = 46 * scale;
    int infoX = 0;
    int infoY = panelY - infoH;
    ctx.ui.drawNarrowTextBar(infoX, infoY, infoSrcW, scale);

    if (selectedMove && moveSelected >= 0 && moveSelected < 4 && moves[moveSelected].moveId >= 0)
    {
        int labelX = infoX + 10 * scale;
        int labelY1 = infoY + 5 * scale;
        int labelY2 = infoY + 24 * scale;

        std::string ppText = "PP " + std::to_string(moves[moveSelected].currentPP) + "-" +
                             std::to_string(moves[moveSelected].maxPP);
        spriteFont.drawText(renderer, ppText, labelX, labelY1, scale);

        std::string typeName = elementTypeName(selectedMove->type);
        if (!typeName.empty())
            typeName[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(typeName[0])));
        spriteFont.drawText(renderer, typeName, labelX, labelY2, scale);
    }
}

void BattleMode::render(GameContext &ctx)
{
    if (!ctx.currentBattle)
        return;

    BattleState bs = ctx.currentBattle->getState();

    // --- Intro animation (no dialogue) ---
    if (bs == BattleState::intro)
    {
        ctx.ui.battleIntroPhase = ctx.currentBattle->getIntroPhase();
        if (ctx.currentBattle->getType() == BattleType::wild)
            drawBattleIntroSceneWild(ctx);
        else
            drawBattleIntroSceneTrainer(ctx);
        return;
    }

    // --- Intro messages (scene backdrop + dialogue) ---
    if (!ctx.currentBattle->isIntroComplete() && bs == BattleState::showingMessages)
    {
        int savedFrame = ctx.ui.battleIntroFrame;
        ctx.ui.battleIntroFrame = GameUI::BATTLE_INTRO_SCENE_DURATION;
        ctx.ui.battleIntroPhase = ctx.currentBattle->getIntroPhase();
        if (ctx.currentBattle->getType() == BattleType::wild)
            drawBattleIntroSceneWild(ctx);
        else
            drawBattleIntroSceneTrainer(ctx);
        ctx.ui.battleIntroFrame = savedFrame;
        ctx.ui.drawDialogueBox("", ctx.currentBattle->getMessage());
        return;
    }

    // --- Normal battle rendering ---
    drawBattleScene(ctx);

    if (bs == BattleState::choosingAction)
    {
        drawBattleMenu(ctx);
    }
    else if (bs == BattleState::choosingMove)
    {
        drawMoveSelectScreen(ctx);
    }
    else if (bs == BattleState::choosingSwitch)
    {
        ctx.ui.drawPartyList(ctx.world.getPlayer(), partySelected);
    }
    else if (bs == BattleState::choosingItem)
    {
        ctx.ui.drawBagScreen(ctx.world.getPlayer(), ctx.pokedex, bagSelected);
    }
    else
    {
        ctx.ui.drawDialogueBox("", ctx.currentBattle->getMessage());
    }
}
