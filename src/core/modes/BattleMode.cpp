#include "BattleMode.h"
#include "../../audio/MusicManager.h"
#include "../../audio/SoundManager.h"
#include "../../battle/Battle.h"
#include "../../data/Pokedex.h"
#include "../../data/Species.h"
#include "../../state/NPC.h"
#include "../../state/Player.h"
#include "../../state/World.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"
#include "../../ui/Renderer.h"
#include "../../ui/SpriteFont.h"
#include "../StringUtils.h"
#include <algorithm>
#include <cmath>

void BattleMode::setTrainerNPCId(const std::string &id) {
    currentTrainerNPCId = id;
}

void BattleMode::setTrainer(std::shared_ptr<NPC> trainer) {
    battleTrainer = std::move(trainer);
}

const std::string &BattleMode::getTrainerNPCId() const {
    return currentTrainerNPCId;
}

void BattleMode::updateBattleIntroAnim(GameContext &ctx) {
    ctx.ui.battleIntroFrame++;
    if (ctx.ui.battleIntroFrame >= GameUI::BATTLE_INTRO_SCENE_DURATION) {
        ctx.currentBattle->finishIntroAnimation();
        ctx.ui.battleIntroFrame = 0;
    }
}

void BattleMode::update(GameContext &ctx, InputManager &input) {
    if (!ctx.currentBattle)
        return;

    battleAnimFrame++;

    BattleState bs = ctx.currentBattle->getState();
    switch (bs) {
    case BattleState::intro:
        updateBattleIntroAnim(ctx);
        return;

    case BattleState::victory:
    case BattleState::defeat:
    case BattleState::fled:
    case BattleState::captured:
        if (bs == BattleState::victory || bs == BattleState::captured) {
            MusicTrack victoryTrack =
                (battleTrainer && battleTrainer->isTrainerType())
                    ? MusicTrack::trainerVictory
                    : MusicTrack::wildVictory;
            ctx.music.play(victoryTrack, ctx.ui.getRenderer().getWindow());
        } else if (bs == BattleState::defeat) {
            ctx.music.stop();
        }

        if (ctx.ui.updateTypewriter(input.isConfirmPressed())) {
            ctx.playSound(SoundEffect::select);
            ctx.pushRequest(ModeRequest::endBattle());
        }
        return;

    case BattleState::showingMessages:
        if (!ctx.currentBattle->isIntroComplete())
            ctx.ui.battleIntroFrame++;

        if (ctx.ui.updateTypewriter(input.isConfirmPressed())) {
            ctx.playSound(SoundEffect::select);
            ctx.currentBattle->advanceMessage();
            if (ctx.currentBattle->getState() == BattleState::intro) {
                ctx.ui.battleIntroFrame = 0;
                ctx.ui.battleIntroPhase = ctx.currentBattle->getIntroPhase();
            }
        }
        return;

    case BattleState::animatingHP: {
        bool done = ctx.ui.tickHPAnimation(
            ctx.currentBattle->getPlayerDaemon().getCurrentHP(),
            ctx.currentBattle->getOpponentDaemon().getCurrentHP(),
            ctx.currentBattle->getPlayerDaemon().getMaxHP(),
            ctx.currentBattle->getOpponentDaemon().getMaxHP());
        if (done)
            ctx.currentBattle->finishHPAnimation();
        return;
    }

    case BattleState::animatingEXP: {
        BattleState bps = ctx.currentBattle->getPendingState();
        if (bps == BattleState::victory || bps == BattleState::captured) {
            MusicTrack victoryTrack =
                (battleTrainer && battleTrainer->isTrainerType())
                    ? MusicTrack::trainerVictory
                    : MusicTrack::wildVictory;
            ctx.music.play(victoryTrack, ctx.ui.getRenderer().getWindow());
        }

        Daemon &daemon = ctx.currentBattle->getPlayerDaemon();
        EXPTickResult result =
            ctx.ui.tickEXPAnimation(daemon.getExp(), daemon.getExpNeeded());

        // Play the EXP sound once at the start of the animation
        if (!expSoundPlayed) {
            ctx.playSound(SoundEffect::expTick);
            expSoundPlayed = true;
        }

        if (result == EXPTickResult::filledBar && daemon.checkLevelUp()) {
            ctx.playSound(SoundEffect::levelUp);
            ctx.ui.playerDisplayEXP = 0;
            ctx.ui.expAnimFrame = 0;
            ctx.ui.expAnimStartEXP = -1;
            expSoundPlayed =
                false; // Reset so sound plays again for next segment
            ctx.currentBattle->addLevelUpMessage(
                daemon.getNickname() + " leveled up to Lv" +
                std::to_string(daemon.getLevel()) + "!");
            ctx.ui.playerDisplayHP = daemon.getCurrentHP();
        } else if (result == EXPTickResult::reachedTarget) {
            ctx.playSound(SoundEffect::expFull);
            expSoundPlayed = false;
            ctx.currentBattle->finishEXPAnimation();
        }
        return;
    }

    case BattleState::opponentTurn:
        ctx.currentBattle->executeOpponentTurn();
        return;

    case BattleState::animatingCapture:
        updateCaptureAnim(ctx);
        return;

    case BattleState::animatingAttack: {
        // Attack timeline (60fps): 0-5 back up, 6-23 hold, 24-29 lunge, 30-35
        // return
        static constexpr int ATTACK_TOTAL_FRAMES = 36;
        attackAnimFrame++;
        if (attackAnimFrame == 6)
            ctx.playSound(SoundEffect::attack);
        if (attackAnimFrame >= ATTACK_TOTAL_FRAMES) {
            attackAnimFrame = 0;
            ctx.currentBattle->finishAttackAnimation();
        }
        return;
    }

    case BattleState::choosingAction:
        ctx.ui.navigate2x2(menuSelected);

        if (input.isConfirmPressed()) {
            ctx.playSound(SoundEffect::select);
            BattleAction actions[] = {BattleAction::fight, BattleAction::item,
                                      BattleAction::switchDaemon,
                                      BattleAction::flee};
            ctx.currentBattle->chooseAction(
                actions[static_cast<std::size_t>(menuSelected)]);
            moveSelected = 0;
            partySelected = 0;
            bagSelected = 0;
            showingPartyAction = false;
            viewingSummary = false;
            partyActionSelected = 0;
        }
        return;

    case BattleState::choosingMove:
        ctx.ui.navigate2x2(moveSelected);

        if (input.isConfirmPressed()) {
            ctx.playSound(SoundEffect::select);
            ctx.currentBattle->chooseMove(moveSelected);
        }
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;

    case BattleState::choosingSwitch:
        if (viewingSummary) {
            // Summary sub-view within battle party
            Direction dir;
            bool dirHeld = input.getMovementDirection(dir);
            if (dirHeld) {
                if (dir == Direction::left && summaryPage > 0)
                    summaryPage = 0;
                else if (dir == Direction::right && summaryPage < 1)
                    summaryPage = 1;
            }
            if (input.isCancelPressed()) {
                viewingSummary = false;
                showingPartyAction = true; // go back to action menu
            }
            return;
        }

        if (showingPartyAction) {
            // Action sub-menu: Summary, Switch, Cancel
            ctx.ui.navigateVertical(partyActionSelected, 3);

            if (input.isConfirmPressed()) {
                ctx.playSound(SoundEffect::select);
                switch (partyActionSelected) {
                case 0: // Summary
                    summaryPage = 0;
                    viewingSummary = true;
                    showingPartyAction = false;
                    break;
                case 1: // Switch
                    showingPartyAction = false;
                    ctx.currentBattle->chooseSwitchTarget(partySelected);
                    break;
                case 2: // Cancel
                    showingPartyAction = false;
                    break;
                }
            }
            if (input.isCancelPressed()) {
                showingPartyAction = false;
            }
            return;
        }

        ctx.ui.navigateVertical(partySelected,
                                ctx.world.getPlayer().partySize());

        if (input.isConfirmPressed()) {
            ctx.playSound(SoundEffect::select);
            showingPartyAction = true;
            partyActionSelected = 0;
        }
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;

    case BattleState::choosingItem: {
        // Reset flag so rendering happens properly.
        captureAnimDone = false;

        int itemCount =
            static_cast<int>(ctx.world.getPlayer().getInventory().size());
        if (itemCount == 0) {
            if (input.isCancelPressed())
                ctx.currentBattle->goBack();
            return;
        }

        ctx.ui.navigateVertical(bagSelected, itemCount);

        if (input.isConfirmPressed()) {
            if (bagSelected >= 0 && bagSelected < itemCount) {
                ctx.playSound(SoundEffect::select);
                const auto &inv = ctx.world.getPlayer().getInventory();
                ctx.currentBattle->chooseItem(
                    inv[static_cast<std::size_t>(bagSelected)].itemId);
                int newSize = static_cast<int>(
                    ctx.world.getPlayer().getInventory().size());
                if (newSize == 0)
                    bagSelected = 0;
                else if (bagSelected >= newSize)
                    bagSelected = newSize - 1;
            }
        }
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;
    }
    }
}

// ── Ball drawing helpers
// ────────────────────────────────────────────────────────

static constexpr int BALL_FRAME_W = 16;
static constexpr int BALL_FRAME_H = 16;

void BattleMode::drawBall(Renderer &renderer, int frame, int x, int y) const {
    renderer.drawSpriteRegion(
        "daemon_ball", frame * BALL_FRAME_W, 0, BALL_FRAME_W, BALL_FRAME_H, x,
        y, BALL_FRAME_W * PIXEL_SCALE, BALL_FRAME_H * PIXEL_SCALE);
}

void BattleMode::drawBallCentered(Renderer &renderer, int frame, int cx,
                                  int cy) const {
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

    // Idle bob: ~6px amplitude, period 60 frames (2s), offset opponent by half
    // period
    constexpr float BOB_PERIOD = 120.0f;
    constexpr float BOB_AMPLITUDE = 6.0f;
    int playerBobY = static_cast<int>(
        std::sin(static_cast<float>(battleAnimFrame) * 6.2832f / BOB_PERIOD) *
        BOB_AMPLITUDE);
    int opponentBobY = static_cast<int>(
        std::sin((static_cast<float>(battleAnimFrame) + BOB_PERIOD / 2.0f) *
                 6.2832f / BOB_PERIOD) *
        BOB_AMPLITUDE);

    // Attack animation offsets (player attacks right, opponent attacks left)
    int playerAttackOffsetX = 0;
    int opponentAttackOffsetX = 0;
    BattleState bs = ctx.currentBattle->getState();
    if (bs == BattleState::animatingAttack) {
        constexpr int BACK_DIST = 16;
        constexpr int LUNGE_DIST = 24;
        int f = attackAnimFrame;
        int offsetX = 0;
        if (f < 6) {
            // Back up: 0→-BACK_DIST over 6 frames
            offsetX = -BACK_DIST * (f + 1) / 6;
        } else if (f < 24) {
            // Hold at backed-up position (0.3s at 60fps)
            offsetX = -BACK_DIST;
        } else if (f < 30) {
            // Lunge: -BACK_DIST → +LUNGE_DIST over 6 frames
            float t = static_cast<float>(f - 24 + 1) / 6.0f;
            offsetX =
                static_cast<int>(-BACK_DIST + (BACK_DIST + LUNGE_DIST) * t);
        } else {
            // Return: +LUNGE_DIST → 0 over 6 frames
            float t = static_cast<float>(f - 30 + 1) / 6.0f;
            offsetX = static_cast<int>(LUNGE_DIST * (1.0f - t));
        }

        if (ctx.currentBattle->isPlayerAttacking())
            playerAttackOffsetX =
                offsetX; // player lunges right (toward opponent)
        else
            opponentAttackOffsetX =
                -offsetX; // opponent lunges left (toward player)
    }

    // Don't draw opponent daemon if it was captured; draw the ball instead
    if (captureAnimDone && ctx.currentBattle->getCaptureSuccess()) {
        auto [baseX, baseY, baseW, baseH] = ui.getOpponentBaseGeometry();
        drawBallCentered(renderer, 0, baseX + baseW / 2, baseY + baseH / 2);
    } else {
        ui.drawOpponentDaemon(opponentDaemon, opponentAttackOffsetX,
                              opponentBobY);
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

    static const std::vector<std::string> options = {"Fight", "Bag", "Daemons",
                                                     "Run"};

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
        spriteFont.drawText(renderer, options[static_cast<std::size_t>(i)], ox,
                            oy, scale);
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

        const MoveData &moveData =
            ctx.pokedex.getMove(moves[static_cast<std::size_t>(i)].moveId);

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
            "PP " +
            std::to_string(
                moves[static_cast<std::size_t>(moveSelected)].currentPP) +
            "-" +
            std::to_string(moves[static_cast<std::size_t>(moveSelected)].maxPP);
        spriteFont.drawText(renderer, ppText, labelX, labelY1, scale);

        std::string typeName =
            StringUtils::capitalize(elementTypeName(selectedMove->type));
        spriteFont.drawText(renderer, typeName, labelX, labelY2, scale);
    }
}

void BattleMode::render(GameContext &ctx) {
    if (!ctx.currentBattle)
        return;

    BattleState bs = ctx.currentBattle->getState();

    // --- Intro animation (no dialogue) ---
    if (bs == BattleState::intro) {
        ctx.ui.battleIntroPhase = ctx.currentBattle->getIntroPhase();
        if (ctx.currentBattle->getType() == BattleType::wild)
            drawBattleIntroSceneWild(ctx);
        else
            drawBattleIntroSceneTrainer(ctx);
        return;
    }

    // --- Intro messages (scene backdrop + dialogue) ---
    if (!ctx.currentBattle->isIntroComplete() &&
        bs == BattleState::showingMessages) {
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

    // --- Capture animation ---
    if (bs == BattleState::animatingCapture) {
        drawCaptureScene(ctx);
        return;
    }

    // --- Normal battle rendering ---
    drawBattleScene(ctx);

    if (bs == BattleState::choosingAction) {
        drawBattleMenu(ctx);
    } else if (bs == BattleState::choosingMove) {
        drawMoveSelectScreen(ctx);
    } else if (bs == BattleState::choosingSwitch) {
        if (viewingSummary) {
            const Daemon &daemon =
                ctx.world.getPlayer().getDaemon(partySelected);
            ctx.ui.drawSummaryScreen(daemon, ctx.pokedex, summaryPage);
        } else {
            ctx.ui.drawPartyList(ctx.world.getPlayer(), partySelected);
            if (showingPartyAction) {
                ctx.ui.drawChoiceBox({"Summary", "Switch", "Cancel"},
                                     partyActionSelected);
            }
        }
    } else if (bs == BattleState::choosingItem) {
        ctx.ui.drawBagScreen(ctx.world.getPlayer(), ctx.pokedex, bagSelected);
    } else {
        ctx.ui.drawDialogueBox("", ctx.currentBattle->getMessage());
    }
}

// ── Capture animation
// ──────────────────────────────────────────────────────────

// Animation timeline (at 60fps):
//   Frames 0-14:   Ball arcs from player toward opponent
//   Frames 15-19:  Ball reaches opponent, daemon disappears
//   For each shake (up to captureShakes):
//     12 frames: ball wobble left-right + pokeball_shake SFX
//   After shakes: if failure → ball opens (pokeball_escape SFX), daemon
//   reappears
//                 if success → ball clicks shut (stays closed)
//   Then finishCaptureAnimation() to continue to messages

static constexpr int CAPTURE_THROW_FRAMES = 30;
static constexpr int CAPTURE_LAND_FRAMES = 10;
static constexpr int CAPTURE_SHAKE_FRAMES = 24;
static constexpr int CAPTURE_SHAKE_PAUSE = 16;

void BattleMode::updateCaptureAnim(GameContext &ctx) {
    captureAnimFrame++;

    int totalShakes = ctx.currentBattle->getCaptureShakes();
    int throwEnd = CAPTURE_THROW_FRAMES;
    int landEnd = throwEnd + CAPTURE_LAND_FRAMES;

    int shakeGroupLen = CAPTURE_SHAKE_FRAMES + CAPTURE_SHAKE_PAUSE;

    // Play shake SFX at the start of each shake phase
    if (captureAnimFrame >= landEnd) {
        int shakeFrame = captureAnimFrame - landEnd;
        int currentShake = shakeFrame / shakeGroupLen;
        int frameInShake = shakeFrame % shakeGroupLen;

        if (currentShake < totalShakes && currentShake < 4) {
            if (frameInShake == 0)
                ctx.playSound(SoundEffect::pokeballShake);
        }
    }

    // Calculate total animation duration
    int shakesEnd = landEnd + std::min(totalShakes, 4) * shakeGroupLen;
    int totalEnd = shakesEnd + 20; // 20 frames for result pause

    if (captureAnimFrame >= totalEnd) {
        if (!ctx.currentBattle->getCaptureSuccess())
            ctx.playSound(SoundEffect::pokeballEscape);

        captureAnimFrame = 0;
        captureAnimShakesDone = 0;
        captureAnimDone = true;
        ctx.currentBattle->finishCaptureAnimation();
    }
}

void BattleMode::drawCaptureScene(GameContext &ctx) {
    GameUI &ui = ctx.ui;
    Renderer &renderer = ui.getRenderer();

    // Draw battle background
    ui.drawBattleBackground();
    ui.drawOpponentBase();
    ui.drawPlayerBase();

    const Daemon *playerDaemon = &ctx.currentBattle->getPlayerDaemon();
    const Daemon *opponentDaemon = &ctx.currentBattle->getOpponentDaemon();
    ui.drawPlayerDaemon(playerDaemon);
    ui.drawPlayerInfoBar(playerDaemon);
    ui.drawOpponentInfoBar(opponentDaemon);

    int totalShakes = ctx.currentBattle->getCaptureShakes();
    bool success = ctx.currentBattle->getCaptureSuccess();

    int shakeGroupLen = CAPTURE_SHAKE_FRAMES + CAPTURE_SHAKE_PAUSE;
    int throwEnd = CAPTURE_THROW_FRAMES;
    int landEnd = throwEnd + CAPTURE_LAND_FRAMES;
    int shakesEnd = landEnd + std::min(totalShakes, 4) * shakeGroupLen;

    // Opponent daemon base position
    auto [baseX, baseY, baseW, baseH] = ui.getOpponentBaseGeometry();

    // Ball destination: center of opponent base
    int ballDstX = baseX + baseW / 2;
    int ballDstY = baseY + baseH / 2;

    // Ball source (from player side)
    int ballSrcX = WINDOW_WIDTH / 4;
    int ballSrcY = WINDOW_HEIGHT - UI_PANEL_HEIGHT - 40 * PIXEL_SCALE;

    if (captureAnimFrame < throwEnd) {
        // Phase: ball arcs toward opponent, daemon still visible
        ui.drawOpponentDaemon(opponentDaemon);

        float t =
            static_cast<float>(captureAnimFrame) / static_cast<float>(throwEnd);
        int ballX = ballSrcX + static_cast<int>(
                                   static_cast<float>(ballDstX - ballSrcX) * t);
        int ballY = ballSrcY + static_cast<int>(
                                   static_cast<float>(ballDstY - ballSrcY) * t);
        // Arc: add parabolic vertical offset
        ballY -= static_cast<int>(120.0f * t * (1.0f - t));

        // Frame 0 = closed ball
        drawBallCentered(renderer, 0, ballX, ballY);
    } else if (captureAnimFrame < landEnd) {
        // Phase: ball landed, daemon disappears
        // Frame 1 = half-open ball
        drawBallCentered(renderer, 1, ballDstX, ballDstY);
    } else if (captureAnimFrame < shakesEnd) {
        // Phase: ball shaking with pauses between shakes
        int shakeFrame = captureAnimFrame - landEnd;
        int currentShake = shakeFrame / shakeGroupLen;
        int frameInGroup = shakeFrame % shakeGroupLen;

        // Wobble left-right only during the active shake portion
        int wobble = 0;
        if (currentShake < std::min(totalShakes, 4) &&
            frameInGroup < CAPTURE_SHAKE_FRAMES) {
            float shakeT = static_cast<float>(frameInGroup) /
                           static_cast<float>(CAPTURE_SHAKE_FRAMES);
            wobble = static_cast<int>(std::sin(shakeT * 6.283f) * 8.0f *
                                      static_cast<float>(PIXEL_SCALE));
        }

        // Frame 0 = closed ball
        drawBallCentered(renderer, 0, ballDstX + wobble, ballDstY);
    } else {
        // Phase: result
        if (success) {
            // Ball stays closed (frame 0)
            drawBallCentered(renderer, 0, ballDstX, ballDstY);
        } else {
            // Ball opens (frame 2 = fully open), daemon reappears
            ui.drawOpponentDaemon(opponentDaemon);
            drawBallCentered(renderer, 2, ballDstX,
                             ballDstY - 10 * PIXEL_SCALE);
        }
    }
}
