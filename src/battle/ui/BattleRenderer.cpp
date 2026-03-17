#include "BattleRenderer.h"
#include "../../common/FilePaths.h"
#include "../../common/StringUtils.h"
#include "../../game_data/Pokedex.h"
#include "../../state/NPC.h"
#include "../../ui/GameUI.h"
#include "../../ui/Renderer.h"
#include "../../ui/SpriteFont.h"
#include "../Battle.h"
#include "BattlePresentationState.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <vector>

namespace {
constexpr int ballFrameW = 16;
constexpr int ballFrameH = 16;
constexpr int captureThrowFrames = 30;
constexpr int captureLandFrames = 10;
constexpr int captureShakeFrames = 24;
constexpr int captureShakePause = 16;

std::string getTrainerFrontTextureId(const NPC &opponent) {
    if (opponent.getSpriteType().empty())
        return {};
    return "npc_front_" + opponent.getSpriteType();
}

std::filesystem::path findTrainerFrontSpritePath(const NPC &opponent) {
    if (opponent.getSpriteType().empty())
        return {};

    const std::string &spriteType = opponent.getSpriteType();
    const std::array<std::filesystem::path, 2> candidates = {
        std::filesystem::path{"assets/sprites/npcs"} / spriteType / (spriteType + "_front.png"),
        std::filesystem::path{"assets/sprites/npcs"} / (spriteType + "_front.png"),
    };

    for (const auto &candidate : candidates) {
        const std::filesystem::path resolved = FilePaths::resolveExistingPath(candidate);
        if (std::filesystem::exists(resolved))
            return resolved;
    }

    return {};
}
} // namespace

void BattleRenderer::drawBall(Renderer &renderer, int frame, int x, int y) const {
    renderer.drawSpriteRegion("daemon_ball", frame * ballFrameW, 0, ballFrameW, ballFrameH, x, y,
                              ballFrameW * PIXEL_SCALE, ballFrameH * PIXEL_SCALE);
}

void BattleRenderer::drawBallCentered(Renderer &renderer, int frame, int cx, int cy) const {
    const int halfW = (ballFrameW * PIXEL_SCALE) / 2;
    const int halfH = (ballFrameH * PIXEL_SCALE) / 2;
    drawBall(renderer, frame, cx - halfW, cy - halfH);
}

void BattleRenderer::drawBattleBackground(Renderer &renderer) const {
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT - UI_PANEL_HEIGHT,
                            TDT4102::Color{200, 220, 200});
}

BattleRenderer::BaseGeometry BattleRenderer::getPlayerBaseGeometry() const {
    const int scale = PIXEL_SCALE;
    const int battleH = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    const int w = 256 * scale;
    const int h = 32 * scale;
    return {-60 * scale, battleH - h, w, h};
}

BattleRenderer::BaseGeometry BattleRenderer::getOpponentBaseGeometry() const {
    const int scale = PIXEL_SCALE;
    const int w = 128 * scale;
    const int h = 64 * scale;
    return {WINDOW_WIDTH - w - 60, 120, w, h};
}

void BattleRenderer::drawPlayerBase(Renderer &renderer) const {
    const auto [x, y, w, h] = getPlayerBaseGeometry();
    renderer.drawSpriteRaw("ui_player_base", x, y, w, h);
}

void BattleRenderer::drawOpponentBase(Renderer &renderer, int offsetX) const {
    const auto [x, y, w, h] = getOpponentBaseGeometry();
    renderer.drawSpriteRaw("ui_opponent_base", x + offsetX, y, w, h);
}

void BattleRenderer::drawPlayerBackSprite(Renderer &renderer, int x, int y, int dstW, int dstH,
                                          int frame) const {
    constexpr int frameW = 80;
    constexpr int frameH = 80;
    constexpr int cols = 3;
    constexpr int totalFrames = 5;

    const int f = frame % totalFrames;
    const int srcX = (f % cols) * frameW;
    const int srcY = (f / cols) * frameH;
    renderer.drawSpriteRegion("player_back", srcX, srcY, frameW, frameH, x, y, dstW, dstH);
}

void BattleRenderer::drawOpponentTrainer(GameUI &ui, const NPC *opponent, int offsetX) const {
    Renderer &renderer = ui.getRenderer();
    SpriteFont &spriteFont = ui.getSpriteFont();
    const int scale = PIXEL_SCALE;
    const auto [baseX, baseY, baseW, baseH] = getOpponentBaseGeometry();

    if (opponent) {
        const std::string textureId = getTrainerFrontTextureId(*opponent);
        if (!textureId.empty() && !renderer.hasTexture(textureId)) {
            const std::filesystem::path spritePath = findTrainerFrontSpritePath(*opponent);
            if (!spritePath.empty())
                renderer.loadTexture(textureId, spritePath);
        }

        if (!textureId.empty() && renderer.hasTexture(textureId)) {
            TDT4102::Image &sprite = renderer.getTexture(textureId);
            const int trainerW = sprite.width * scale;
            const int trainerH = sprite.height * scale;
            const int trainerX = baseX + baseW / 2 - trainerW / 2 + offsetX;
            const int trainerY = baseY - trainerH + baseH / 2;
            renderer.drawSpriteRaw(textureId, trainerX, trainerY, trainerW, trainerH);
            return;
        }
    }

    const int trainerW = 40 * scale;
    const int trainerH = 56 * scale;
    const int trainerX = baseX + baseW / 2 - trainerW / 2 + offsetX;
    const int trainerY = baseY + baseH - trainerH - 10 * scale;
    renderer.drawFilledRect(trainerX, trainerY, trainerW, trainerH, TDT4102::Color{60, 60, 80});
    if (opponent) {
        spriteFont.drawText(renderer, opponent->getName(), trainerX + 2 * scale,
                            trainerY + trainerH / 2, scale - 1);
    }
}

void BattleRenderer::drawOpponentInfoBar(GameUI &ui, const Daemon *opponentDaemon,
                                         const BattlePresentationState &presentation,
                                         int offsetX) const {
    Renderer &renderer = ui.getRenderer();
    SpriteFont &spriteFont = ui.getSpriteFont();
    const int scale = PIXEL_SCALE;

    const int oppPanelW = 122 * scale;
    const int oppPanelH = 35 * scale;
    const int oppPanelX = offsetX;
    const int oppPanelY = 16;

    renderer.drawSpriteRaw("ui_opponent_info", oppPanelX, oppPanelY, oppPanelW, oppPanelH);

    if (!opponentDaemon)
        return;

    const int oppNameX = oppPanelX + 2 * scale;
    const int oppNameY = oppPanelY + 9 * scale;
    spriteFont.drawText(renderer, opponentDaemon->getNickname(), oppNameX, oppNameY, scale);

    const int oppLvlX = oppPanelX + 83 * scale;
    const int oppLvlY = oppPanelY + 12 * scale;
    spriteFont.drawBattleNumber(renderer, opponentDaemon->getLevel(), oppLvlX, oppLvlY, scale);

    const int oppHPBarX = oppPanelX + 50 * scale;
    const int oppHPBarY = oppPanelY + 24 * scale;
    const int oppHPBarW = 48 * scale;
    ui.drawSpriteHPBar(oppHPBarX, oppHPBarY, oppHPBarW, presentation.opponentDisplayHP,
                       opponentDaemon->getMaxHP(), scale);
}

void BattleRenderer::drawPlayerInfoBar(GameUI &ui, const Daemon *playerDaemon,
                                       const BattlePresentationState &presentation,
                                       int offsetX) const {
    Renderer &renderer = ui.getRenderer();
    SpriteFont &spriteFont = ui.getSpriteFont();
    const int scale = PIXEL_SCALE;
    const int battleH = WINDOW_HEIGHT - UI_PANEL_HEIGHT;

    const int plyPanelW = 128 * scale;
    const int plyPanelH = 47 * scale;
    const int plyPanelX = WINDOW_WIDTH - plyPanelW + offsetX;
    const int plyPanelY = battleH - plyPanelH - 10;

    renderer.drawSpriteRaw("ui_player_info", plyPanelX, plyPanelY, plyPanelW, plyPanelH);

    if (!playerDaemon)
        return;

    const int plyNameX = plyPanelX + 18 * scale;
    const int plyNameY = plyPanelY + 10 * scale;
    spriteFont.drawText(renderer, playerDaemon->getNickname(), plyNameX, plyNameY, scale);

    const int plyLvlX = plyPanelX + 105 * scale;
    const int plyLvlY = plyPanelY + 12 * scale;
    spriteFont.drawBattleNumber(renderer, playerDaemon->getLevel(), plyLvlX, plyLvlY, scale);

    const int plyHPBarX = plyPanelX + 72 * scale;
    const int plyHPBarY = plyPanelY + 25 * scale;
    const int plyHPBarW = 48 * scale;
    ui.drawSpriteHPBar(plyHPBarX, plyHPBarY, plyHPBarW, presentation.playerDisplayHP,
                       playerDaemon->getMaxHP(), scale);

    const int hpNumX = plyPanelX + 92 * scale;
    const int hpNumY = plyPanelY + 32 * scale;
    const int hpPad = 3 * scale;
    spriteFont.drawBattleNumber(renderer, presentation.playerDisplayHP, hpNumX - hpPad, hpNumY,
                                scale, true);
    spriteFont.drawBattleNumber(renderer, playerDaemon->getMaxHP(), hpNumX + hpPad, hpNumY, scale);

    const int expBarX = plyPanelX + 24 * scale;
    const int expBarY = plyPanelY + 42 * scale;
    const int expBarW = 96 * scale;
    ui.drawSpriteEXPBar(expBarX, expBarY, expBarW, presentation.playerDisplayEXP,
                        playerDaemon->getExpNeeded(), scale);
}

void BattleRenderer::drawOpponentDaemon(GameUI &ui, const Daemon *opponentDaemon, int offsetX,
                                        int offsetY) const {
    if (!opponentDaemon)
        return;

    ui.loadDaemonSprite(opponentDaemon->getSpecies().name);
    Renderer &renderer = ui.getRenderer();
    const auto [baseX, baseY, baseW, baseH] = getOpponentBaseGeometry();
    const int scale = PIXEL_SCALE;

    const std::string spriteId = "daemon_" + opponentDaemon->getSpecies().name;
    TDT4102::Image &sprite = renderer.getTexture(spriteId);
    const int spriteW = sprite.width * scale;
    const int spriteH = sprite.height * scale;
    const int spriteX = baseX + baseW / 2 - spriteW / 2 + offsetX;
    const int spriteY = baseY - spriteH + baseH - 10 * scale + offsetY;
    renderer.drawSpriteRaw(spriteId, spriteX, spriteY, spriteW, spriteH);
}

void BattleRenderer::drawPlayerDaemon(GameUI &ui, const Daemon *playerDaemon, int offsetX,
                                      int offsetY) const {
    if (!playerDaemon)
        return;

    ui.loadDaemonSprite(playerDaemon->getSpecies().name);
    Renderer &renderer = ui.getRenderer();
    const auto [baseX, baseY, baseW, baseH] = getPlayerBaseGeometry();
    const int scale = PIXEL_SCALE;

    const std::string spriteId = "daemon_" + playerDaemon->getSpecies().name + "_back";
    TDT4102::Image &sprite = renderer.getTexture(spriteId);
    const int spriteW = sprite.width * scale;
    const int spriteH = sprite.height * scale;
    const int spriteX = baseX + baseW / 2 - spriteW / 2 + offsetX;
    const int spriteY = baseY - spriteH + baseH + offsetY;
    renderer.drawSpriteRaw(spriteId, spriteX, spriteY, spriteW, spriteH);
}

void BattleRenderer::drawPlayerBackOnBase(Renderer &renderer, int offsetX, int frame) const {
    const auto [baseX, baseY, baseW, baseH] = getPlayerBaseGeometry();
    const int scale = PIXEL_SCALE;
    const int backW = 80 * scale;
    const int backH = 80 * scale;
    const int backX = baseX + baseW / 2 - backW / 2 + offsetX;
    const int backY = baseY - backH + baseH;
    drawPlayerBackSprite(renderer, backX, backY, backW, backH, frame);
}

void BattleRenderer::drawPlayerSendOutPhase(GameUI &ui, const Daemon *playerDaemon,
                                            const BattlePresentationState &presentation,
                                            float t) const {
    Renderer &renderer = ui.getRenderer();
    const int playerSlideOut = static_cast<int>(-WINDOW_WIDTH * t);
    const int throwFrame = presentation.introFrame / 4;
    if (t < 0.5f)
        drawPlayerBackOnBase(renderer, playerSlideOut, throwFrame);

    const int daemonSlideIn = static_cast<int>(-WINDOW_WIDTH * (1.0f - t));
    drawPlayerDaemon(ui, playerDaemon, daemonSlideIn);

    const int infoSlideIn = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
    drawPlayerInfoBar(ui, playerDaemon, presentation, infoSlideIn);
}

void BattleRenderer::drawBattleScene(GameUI &ui, Battle &battle,
                                     const BattlePresentationState &presentation,
                                     int battleAnimFrame, int attackAnimFrame,
                                     bool captureAnimDone) const {
    ui.loadBattleAssets();
    Renderer &renderer = ui.getRenderer();
    drawBattleBackground(renderer);
    drawOpponentBase(renderer);
    drawPlayerBase(renderer);

    const Daemon *playerDaemon = &battle.getPlayerDaemon();
    const Daemon *opponentDaemon = &battle.getOpponentDaemon();

    constexpr float bobPeriod = 120.0f;
    constexpr float bobAmplitude = 6.0f;
    const int playerBobY = static_cast<int>(
        std::sin(static_cast<float>(battleAnimFrame) * 6.2832f / bobPeriod) * bobAmplitude);
    const int opponentBobY = static_cast<int>(
        std::sin((static_cast<float>(battleAnimFrame) + bobPeriod / 2.0f) * 6.2832f / bobPeriod) *
        bobAmplitude);

    int playerAttackOffsetX = 0;
    int opponentAttackOffsetX = 0;
    if (battle.getState() == BattleState::animatingAttack) {
        constexpr int backDist = 16;
        constexpr int lungeDist = 24;
        int offsetX = 0;
        if (attackAnimFrame < 6) {
            offsetX = -backDist * (attackAnimFrame + 1) / 6;
        } else if (attackAnimFrame < 24) {
            offsetX = -backDist;
        } else if (attackAnimFrame < 30) {
            const float t = static_cast<float>(attackAnimFrame - 24 + 1) / 6.0f;
            offsetX = static_cast<int>(-backDist + (backDist + lungeDist) * t);
        } else {
            const float t = static_cast<float>(attackAnimFrame - 30 + 1) / 6.0f;
            offsetX = static_cast<int>(lungeDist * (1.0f - t));
        }

        if (battle.isPlayerAttacking())
            playerAttackOffsetX = offsetX;
        else
            opponentAttackOffsetX = -offsetX;
    }

    if (captureAnimDone && battle.getCaptureSuccess()) {
        const auto [baseX, baseY, baseW, baseH] = getOpponentBaseGeometry();
        drawBallCentered(renderer, 0, baseX + baseW / 2, baseY + baseH / 2);
    } else {
        drawOpponentDaemon(ui, opponentDaemon, opponentAttackOffsetX, opponentBobY);
    }
    drawPlayerDaemon(ui, playerDaemon, playerAttackOffsetX, playerBobY);
    drawOpponentInfoBar(ui, opponentDaemon, presentation);
    drawPlayerInfoBar(ui, playerDaemon, presentation);
}

void BattleRenderer::drawBattleIntroSceneWild(GameUI &ui, Battle &battle,
                                              const BattlePresentationState &presentation) const {
    ui.loadBattleAssets();
    Renderer &renderer = ui.getRenderer();
    drawBattleBackground(renderer);

    float t = static_cast<float>(presentation.introFrame) /
              static_cast<float>(BattlePresentationState::introSceneDuration);
    if (t > 1.0f)
        t = 1.0f;

    const Daemon *playerDaemon = &battle.getPlayerDaemon();
    const Daemon *opponentDaemon = &battle.getOpponentDaemon();

    if (presentation.introPhase == 0) {
        drawOpponentBase(renderer);
        drawPlayerBase(renderer);
        const int slideOffset = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        drawOpponentDaemon(ui, opponentDaemon, slideOffset);
        drawPlayerBackOnBase(renderer);
    } else if (presentation.introPhase == 1) {
        drawOpponentBase(renderer);
        drawPlayerBase(renderer);
        drawOpponentDaemon(ui, opponentDaemon);
        drawOpponentInfoBar(ui, opponentDaemon, presentation);
        drawPlayerSendOutPhase(ui, playerDaemon, presentation, t);
    }
}

void BattleRenderer::drawBattleIntroSceneTrainer(
    GameUI &ui, Battle &battle, const BattlePresentationState &presentation) const {
    ui.loadBattleAssets();
    Renderer &renderer = ui.getRenderer();
    drawBattleBackground(renderer);

    float t = static_cast<float>(presentation.introFrame) /
              static_cast<float>(BattlePresentationState::introSceneDuration);
    if (t > 1.0f)
        t = 1.0f;

    const Daemon *playerDaemon = &battle.getPlayerDaemon();
    const Daemon *opponentDaemon = &battle.getOpponentDaemon();
    NPC *opponent = battle.getOpponent();

    if (presentation.introPhase == 0) {
        drawOpponentBase(renderer);
        drawPlayerBase(renderer);
        const int slideOffset = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        drawOpponentTrainer(ui, opponent, slideOffset);
        drawPlayerBackOnBase(renderer);
    } else if (presentation.introPhase == 1) {
        drawOpponentBase(renderer);
        drawPlayerBase(renderer);
        const int trainerOut = static_cast<int>(WINDOW_WIDTH * t);
        drawOpponentTrainer(ui, opponent, trainerOut);
        const int daemonIn = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
        drawOpponentDaemon(ui, opponentDaemon, daemonIn);
        drawPlayerBackOnBase(renderer);
    } else if (presentation.introPhase == 2) {
        drawOpponentBase(renderer);
        drawPlayerBase(renderer);
        drawOpponentDaemon(ui, opponentDaemon);
        drawOpponentInfoBar(ui, opponentDaemon, presentation);
        drawPlayerSendOutPhase(ui, playerDaemon, presentation, t);
    }
}

void BattleRenderer::drawBattleMenu(GameUI &ui, int menuSelected) const {
    Renderer &renderer = ui.getRenderer();
    SpriteFont &spriteFont = ui.getSpriteFont();
    static const std::vector<std::string> options = {"Fight", "Bag", "Daemons", "Run"};

    const int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    ui.drawTextBar(panelY);

    const int menuX = WINDOW_WIDTH / 2;
    const int menuWidth = WINDOW_WIDTH / 2;
    const int optionHeight = UI_PANEL_HEIGHT / 2 - 10;
    const int scale = PIXEL_SCALE;
    const int optionCount = std::min(static_cast<int>(options.size()), 4);

    for (int i = 0; i < optionCount; ++i) {
        const int col = i % 2;
        const int row = i / 2;
        const int ox = menuX + col * (menuWidth / 2) + 20;
        const int oy = panelY + row * optionHeight + 23;

        if (i == menuSelected)
            ui.drawSelectionArrow(ox - 16, oy + 4 * scale, scale);
        spriteFont.drawText(renderer, options[static_cast<std::size_t>(i)], ox, oy, scale);
    }
}

void BattleRenderer::drawMoveSelectScreen(GameUI &ui, const Daemon &daemon, const Pokedex &pokedex,
                                          int moveSelected) const {
    Renderer &renderer = ui.getRenderer();
    SpriteFont &spriteFont = ui.getSpriteFont();

    const int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    const int scale = PIXEL_SCALE;
    ui.drawTextBar(panelY);

    const int textBarW = 252 * scale;
    const int textBarX = (WINDOW_WIDTH - textBarW) / 2;
    const int gridX = textBarX + 8 * scale;
    const int gridY = panelY + (UI_PANEL_HEIGHT - 46 * scale) / 2 + 5 * scale;
    const int colW = (textBarW / 2) - 12 * scale;
    const int rowH = 18 * scale;

    const auto &moves = daemon.getMoves();
    const MoveData *selectedMove = nullptr;

    for (int i = 0; i < 4; ++i) {
        const int col = i % 2;
        const int row = i / 2;
        const int ox = gridX + col * colW;
        const int oy = gridY + row * rowH;

        if (moves[static_cast<std::size_t>(i)].moveId < 0) {
            spriteFont.drawText(renderer, "---", ox + 6 * scale, oy, scale);
            continue;
        }

        const MoveData &moveData = pokedex.getMove(moves[static_cast<std::size_t>(i)].moveId);
        if (i == moveSelected) {
            selectedMove = &moveData;
            ui.drawSelectionArrow(ox + scale, oy + 4 * scale, scale);
        }
        spriteFont.drawText(renderer, moveData.name, ox + 6 * scale, oy, scale);
    }

    const int infoSrcW = 100;
    const int infoH = 46 * scale;
    const int infoX = 0;
    const int infoY = panelY - infoH;
    ui.drawNarrowTextBar(infoX, infoY, infoSrcW, scale);

    if (selectedMove && moveSelected >= 0 && moveSelected < 4 &&
        moves[static_cast<std::size_t>(moveSelected)].moveId >= 0) {
        const int labelX = infoX + 10 * scale;
        const int labelY1 = infoY + 5 * scale;
        const int labelY2 = infoY + 24 * scale;

        const std::string ppText =
            "PP " + std::to_string(moves[static_cast<std::size_t>(moveSelected)].currentPP) + "-" +
            std::to_string(moves[static_cast<std::size_t>(moveSelected)].maxPP);
        spriteFont.drawText(renderer, ppText, labelX, labelY1, scale);

        const std::string typeName = StringUtils::capitalize(elementTypeName(selectedMove->type));
        spriteFont.drawText(renderer, typeName, labelX, labelY2, scale);
    }
}

void BattleRenderer::drawCaptureScene(GameUI &ui, Battle &battle,
                                      const BattlePresentationState &presentation,
                                      int captureAnimFrame, bool captureAnimDone) const {
    ui.loadBattleAssets();
    Renderer &renderer = ui.getRenderer();
    drawBattleBackground(renderer);
    drawOpponentBase(renderer);
    drawPlayerBase(renderer);

    const Daemon *playerDaemon = &battle.getPlayerDaemon();
    const Daemon *opponentDaemon = &battle.getOpponentDaemon();
    drawPlayerDaemon(ui, playerDaemon);
    drawPlayerInfoBar(ui, playerDaemon, presentation);
    drawOpponentInfoBar(ui, opponentDaemon, presentation);

    const int totalShakes = battle.getCaptureShakes();
    const bool success = battle.getCaptureSuccess();
    const int shakeGroupLen = captureShakeFrames + captureShakePause;
    const int throwEnd = captureThrowFrames;
    const int landEnd = throwEnd + captureLandFrames;
    const int shakesEnd = landEnd + std::min(totalShakes, 4) * shakeGroupLen;

    const auto [baseX, baseY, baseW, baseH] = getOpponentBaseGeometry();
    const int ballDstX = baseX + baseW / 2;
    const int ballDstY = baseY + baseH / 2;
    const int ballSrcX = WINDOW_WIDTH / 4;
    const int ballSrcY = WINDOW_HEIGHT - UI_PANEL_HEIGHT - 40 * PIXEL_SCALE;

    if (captureAnimFrame < throwEnd) {
        drawOpponentDaemon(ui, opponentDaemon);

        const float t = static_cast<float>(captureAnimFrame) / static_cast<float>(throwEnd);
        const int ballX = ballSrcX + static_cast<int>(static_cast<float>(ballDstX - ballSrcX) * t);
        int ballY = ballSrcY + static_cast<int>(static_cast<float>(ballDstY - ballSrcY) * t);
        ballY -= static_cast<int>(120.0f * t * (1.0f - t));

        drawBallCentered(renderer, 0, ballX, ballY);
    } else if (captureAnimFrame < landEnd) {
        drawBallCentered(renderer, 1, ballDstX, ballDstY);
    } else if (captureAnimFrame < shakesEnd) {
        const int shakeFrame = captureAnimFrame - landEnd;
        const int currentShake = shakeFrame / shakeGroupLen;
        const int frameInGroup = shakeFrame % shakeGroupLen;

        int wobble = 0;
        if (currentShake < std::min(totalShakes, 4) && frameInGroup < captureShakeFrames) {
            const float shakeT =
                static_cast<float>(frameInGroup) / static_cast<float>(captureShakeFrames);
            wobble = static_cast<int>(std::sin(shakeT * 6.283f) * 8.0f *
                                      static_cast<float>(PIXEL_SCALE));
        }

        drawBallCentered(renderer, 0, ballDstX + wobble, ballDstY);
    } else if (captureAnimDone && success) {
        drawBallCentered(renderer, 0, ballDstX, ballDstY);
    } else if (success) {
        drawBallCentered(renderer, 0, ballDstX, ballDstY);
    } else {
        drawOpponentDaemon(ui, opponentDaemon);
        drawBallCentered(renderer, 2, ballDstX, ballDstY - 10 * PIXEL_SCALE);
    }
}
