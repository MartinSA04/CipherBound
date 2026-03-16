#include "../../../battle/Battle.h"
#include "../../../data/Pokedex.h"
#include "../../../data/Species.h"
#include "../../../state/NPC.h"
#include "../../../state/World.h"
#include "../../../state/player/Player.h"
#include "../../../ui/GameUI.h"
#include "../../../ui/Renderer.h"
#include "../../../ui/SpriteFont.h"
#include "../../StringUtils.h"
#include "../BattleMode.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr int BALL_FRAME_W = 16;
constexpr int BALL_FRAME_H = 16;
} // namespace

void BattleMode::drawBall(Renderer &renderer, int frame, int x, int y) const {
    renderer.drawSpriteRegion("daemon_ball", frame * BALL_FRAME_W, 0, BALL_FRAME_W, BALL_FRAME_H, x,
                              y, BALL_FRAME_W * PIXEL_SCALE, BALL_FRAME_H * PIXEL_SCALE);
}

void BattleMode::drawBallCentered(Renderer &renderer, int frame, int cx, int cy) const {
    int halfW = (BALL_FRAME_W * PIXEL_SCALE) / 2;
    int halfH = (BALL_FRAME_H * PIXEL_SCALE) / 2;
    drawBall(renderer, frame, cx - halfW, cy - halfH);
}

void BattleMode::drawBattleScene(GameContext &ctx) {
    GameUI &ui = ctx.ui;
    ui.loadBattleAssets();

    Renderer &renderer = ui.getRenderer();
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT - UI_PANEL_HEIGHT,
                            TDT4102::Color{200, 220, 200});

    ui.drawOpponentBase();
    ui.drawPlayerBase();

    const Daemon *playerDaemon = &ctx.currentBattle->getPlayerDaemon();
    const Daemon *opponentDaemon = &ctx.currentBattle->getOpponentDaemon();

    constexpr float BOB_PERIOD = 120.0f;
    constexpr float BOB_AMPLITUDE = 6.0f;
    int playerBobY = static_cast<int>(
        std::sin(static_cast<float>(battleAnimFrame) * 6.2832f / BOB_PERIOD) * BOB_AMPLITUDE);
    int opponentBobY = static_cast<int>(
        std::sin((static_cast<float>(battleAnimFrame) + BOB_PERIOD / 2.0f) * 6.2832f / BOB_PERIOD) *
        BOB_AMPLITUDE);

    int playerAttackOffsetX = 0;
    int opponentAttackOffsetX = 0;
    BattleState bs = ctx.currentBattle->getState();
    if (bs == BattleState::animatingAttack) {
        constexpr int BACK_DIST = 16;
        constexpr int LUNGE_DIST = 24;
        int f = attackAnimFrame;
        int offsetX = 0;
        if (f < 6) {
            offsetX = -BACK_DIST * (f + 1) / 6;
        } else if (f < 24) {
            offsetX = -BACK_DIST;
        } else if (f < 30) {
            float t = static_cast<float>(f - 24 + 1) / 6.0f;
            offsetX = static_cast<int>(-BACK_DIST + (BACK_DIST + LUNGE_DIST) * t);
        } else {
            float t = static_cast<float>(f - 30 + 1) / 6.0f;
            offsetX = static_cast<int>(LUNGE_DIST * (1.0f - t));
        }

        if (ctx.currentBattle->isPlayerAttacking())
            playerAttackOffsetX = offsetX;
        else
            opponentAttackOffsetX = -offsetX;
    }

    if (captureAnimDone && ctx.currentBattle->getCaptureSuccess()) {
        auto [baseX, baseY, baseW, baseH] = ui.getOpponentBaseGeometry();
        drawBallCentered(renderer, 0, baseX + baseW / 2, baseY + baseH / 2);
    } else {
        ui.drawOpponentDaemon(opponentDaemon, opponentAttackOffsetX, opponentBobY);
    }
    ui.drawPlayerDaemon(playerDaemon, playerAttackOffsetX, playerBobY);
    ui.drawOpponentInfoBar(opponentDaemon);
    ui.drawPlayerInfoBar(playerDaemon);
}

void BattleMode::drawBattleIntroSceneWild(GameContext &ctx) {
    GameUI &ui = ctx.ui;
    ui.loadBattleAssets();

    float t = static_cast<float>(ui.battleIntroFrame) /
              static_cast<float>(GameUI::BATTLE_INTRO_SCENE_DURATION);
    if (t > 1.0f)
        t = 1.0f;

    ui.drawBattleBackground();

    const Daemon *playerDaemon = &ctx.currentBattle->getPlayerDaemon();
    const Daemon *opponentDaemon = &ctx.currentBattle->getOpponentDaemon();

    if (ui.battleIntroPhase == 0) {
        ui.drawOpponentBase();
        ui.drawPlayerBase();
        int slideOffset = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        ui.drawOpponentDaemon(opponentDaemon, slideOffset);
        ui.drawPlayerBackOnBase();
    } else if (ui.battleIntroPhase == 1) {
        ui.drawOpponentBase();
        ui.drawPlayerBase();
        ui.drawOpponentDaemon(opponentDaemon);
        ui.drawOpponentInfoBar(opponentDaemon);
        ui.drawPlayerSendOutPhase(playerDaemon, t);
    }
}

void BattleMode::drawBattleIntroSceneTrainer(GameContext &ctx) {
    GameUI &ui = ctx.ui;
    ui.loadBattleAssets();

    float t = static_cast<float>(ui.battleIntroFrame) /
              static_cast<float>(GameUI::BATTLE_INTRO_SCENE_DURATION);
    if (t > 1.0f)
        t = 1.0f;

    ui.drawBattleBackground();

    const Daemon *playerDaemon = &ctx.currentBattle->getPlayerDaemon();
    const Daemon *opponentDaemon = &ctx.currentBattle->getOpponentDaemon();
    NPC *opponent = ctx.currentBattle->getOpponent().get();

    if (ui.battleIntroPhase == 0) {
        ui.drawOpponentBase();
        ui.drawPlayerBase();
        int slideOffset = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        ui.drawOpponentTrainer(opponent, slideOffset);
        ui.drawPlayerBackOnBase();
    } else if (ui.battleIntroPhase == 1) {
        ui.drawOpponentBase();
        ui.drawPlayerBase();
        int trainerOut = static_cast<int>(WINDOW_WIDTH * t);
        ui.drawOpponentTrainer(opponent, trainerOut);
        int daemonIn = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        ui.drawOpponentDaemon(opponentDaemon, daemonIn);
        ui.drawPlayerBackOnBase();
    } else if (ui.battleIntroPhase == 2) {
        ui.drawOpponentBase();
        ui.drawPlayerBase();
        ui.drawOpponentDaemon(opponentDaemon);
        ui.drawOpponentInfoBar(opponentDaemon);
        ui.drawPlayerSendOutPhase(playerDaemon, t);
    }
}

void BattleMode::drawBattleMenu(GameContext &ctx) {
    Renderer &renderer = ctx.ui.getRenderer();
    SpriteFont &spriteFont = ctx.ui.getSpriteFont();

    static const std::vector<std::string> options = {"Fight", "Bag", "Daemons", "Run"};

    int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    ctx.ui.drawTextBar(panelY);

    int menuX = WINDOW_WIDTH / 2;
    int menuWidth = WINDOW_WIDTH / 2;
    int optionHeight = UI_PANEL_HEIGHT / 2 - 10;
    int scale = PIXEL_SCALE;

    int optionCount = std::min(static_cast<int>(options.size()), 4);
    for (int i = 0; i < optionCount; ++i) {
        int col = i % 2;
        int row = i / 2;
        int ox = menuX + col * (menuWidth / 2) + 20;
        int oy = panelY + row * optionHeight + 23;

        if (i == menuSelected) {
            ctx.ui.drawSelectionArrow(ox - 16, oy + 4 * scale, scale);
        }
        spriteFont.drawText(renderer, options[static_cast<std::size_t>(i)], ox, oy, scale);
    }
}

void BattleMode::drawMoveSelectScreen(GameContext &ctx) {
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

    for (int i = 0; i < 4; ++i) {
        int col = i % 2;
        int row = i / 2;
        int ox = gridX + col * colW;
        int oy = gridY + row * rowH;

        if (moves[static_cast<std::size_t>(i)].moveId < 0) {
            spriteFont.drawText(renderer, "---", ox + 6 * scale, oy, scale);
            continue;
        }

        const MoveData &moveData = ctx.pokedex.getMove(moves[static_cast<std::size_t>(i)].moveId);

        if (i == moveSelected) {
            selectedMove = &moveData;
            ctx.ui.drawSelectionArrow(ox + scale, oy + 4 * scale, scale);
        }

        spriteFont.drawText(renderer, moveData.name, ox + 6 * scale, oy, scale);
    }

    int infoSrcW = 100;
    int infoH = 46 * scale;
    int infoX = 0;
    int infoY = panelY - infoH;
    ctx.ui.drawNarrowTextBar(infoX, infoY, infoSrcW, scale);

    if (selectedMove && moveSelected >= 0 && moveSelected < 4 &&
        moves[static_cast<std::size_t>(moveSelected)].moveId >= 0) {
        int labelX = infoX + 10 * scale;
        int labelY1 = infoY + 5 * scale;
        int labelY2 = infoY + 24 * scale;

        std::string ppText =
            "PP " + std::to_string(moves[static_cast<std::size_t>(moveSelected)].currentPP) + "-" +
            std::to_string(moves[static_cast<std::size_t>(moveSelected)].maxPP);
        spriteFont.drawText(renderer, ppText, labelX, labelY1, scale);

        std::string typeName = StringUtils::capitalize(elementTypeName(selectedMove->type));
        spriteFont.drawText(renderer, typeName, labelX, labelY2, scale);
    }
}

void BattleMode::render(GameContext &ctx) {
    if (!ctx.currentBattle)
        return;

    BattleState bs = ctx.currentBattle->getState();

    if (bs == BattleState::intro) {
        ctx.ui.battleIntroPhase = ctx.currentBattle->getIntroPhase();
        if (ctx.currentBattle->getType() == BattleType::wild)
            drawBattleIntroSceneWild(ctx);
        else
            drawBattleIntroSceneTrainer(ctx);
        return;
    }

    if (!ctx.currentBattle->isIntroComplete() && bs == BattleState::showingMessages) {
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

    if (bs == BattleState::animatingCapture) {
        drawCaptureScene(ctx);
        return;
    }

    drawBattleScene(ctx);

    if (bs == BattleState::choosingAction) {
        drawBattleMenu(ctx);
    } else if (bs == BattleState::choosingMove) {
        drawMoveSelectScreen(ctx);
    } else if (bs == BattleState::choosingSwitch) {
        if (viewingSummary) {
            const Daemon &daemon = ctx.world.getPlayer().getDaemon(partySelected);
            ctx.ui.drawSummaryScreen(daemon, ctx.pokedex, summaryPage);
        } else {
            ctx.ui.drawPartyList(ctx.world.getPlayer(), partySelected);
            if (showingPartyAction) {
                ctx.ui.drawChoiceBox({"Summary", "Switch", "Cancel"}, partyActionSelected);
            }
        }
    } else if (bs == BattleState::choosingItem) {
        ctx.ui.drawBagScreen(ctx.world.getPlayer(), ctx.pokedex, bagSelected);
    } else {
        ctx.ui.drawDialogueBox("", ctx.currentBattle->getMessage());
    }
}
