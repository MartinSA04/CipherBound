#include "../../../audio/SoundManager.h"
#include "../../../battle/Battle.h"
#include "../../../ui/GameUI.h"
#include "../../../ui/Renderer.h"
#include "../BattleMode.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr int CAPTURE_THROW_FRAMES = 30;
constexpr int CAPTURE_LAND_FRAMES = 10;
constexpr int CAPTURE_SHAKE_FRAMES = 24;
constexpr int CAPTURE_SHAKE_PAUSE = 16;
} // namespace

void BattleMode::updateCaptureAnim(GameContext &ctx) {
    Battle &battle = ctx.battle();
    captureAnimFrame++;

    int totalShakes = battle.getCaptureShakes();
    int throwEnd = CAPTURE_THROW_FRAMES;
    int landEnd = throwEnd + CAPTURE_LAND_FRAMES;

    int shakeGroupLen = CAPTURE_SHAKE_FRAMES + CAPTURE_SHAKE_PAUSE;

    if (captureAnimFrame >= landEnd) {
        int shakeFrame = captureAnimFrame - landEnd;
        int currentShake = shakeFrame / shakeGroupLen;
        int frameInShake = shakeFrame % shakeGroupLen;

        if (currentShake < totalShakes && currentShake < 4) {
            if (frameInShake == 0)
                ctx.playSound(SoundEffect::pokeballShake);
        }
    }

    int shakesEnd = landEnd + std::min(totalShakes, 4) * shakeGroupLen;
    int totalEnd = shakesEnd + 20;

    if (captureAnimFrame >= totalEnd) {
        if (!battle.getCaptureSuccess())
            ctx.playSound(SoundEffect::pokeballEscape);

        captureAnimFrame = 0;
        captureAnimShakesDone = 0;
        captureAnimDone = true;
        battle.finishCaptureAnimation();
    }
}

void BattleMode::drawCaptureScene(GameContext &ctx) {
    GameUI &ui = ctx.ui;
    Renderer &renderer = ui.getRenderer();
    Battle &battle = ctx.battle();

    ui.drawBattleBackground();
    ui.drawOpponentBase();
    ui.drawPlayerBase();

    const Daemon *playerDaemon = &battle.getPlayerDaemon();
    const Daemon *opponentDaemon = &battle.getOpponentDaemon();
    ui.drawPlayerDaemon(playerDaemon);
    ui.drawPlayerInfoBar(playerDaemon);
    ui.drawOpponentInfoBar(opponentDaemon);

    int totalShakes = battle.getCaptureShakes();
    bool success = battle.getCaptureSuccess();

    int shakeGroupLen = CAPTURE_SHAKE_FRAMES + CAPTURE_SHAKE_PAUSE;
    int throwEnd = CAPTURE_THROW_FRAMES;
    int landEnd = throwEnd + CAPTURE_LAND_FRAMES;
    int shakesEnd = landEnd + std::min(totalShakes, 4) * shakeGroupLen;

    auto [baseX, baseY, baseW, baseH] = ui.getOpponentBaseGeometry();

    int ballDstX = baseX + baseW / 2;
    int ballDstY = baseY + baseH / 2;

    int ballSrcX = WINDOW_WIDTH / 4;
    int ballSrcY = WINDOW_HEIGHT - UI_PANEL_HEIGHT - 40 * PIXEL_SCALE;

    if (captureAnimFrame < throwEnd) {
        ui.drawOpponentDaemon(opponentDaemon);

        float t = static_cast<float>(captureAnimFrame) / static_cast<float>(throwEnd);
        int ballX = ballSrcX + static_cast<int>(static_cast<float>(ballDstX - ballSrcX) * t);
        int ballY = ballSrcY + static_cast<int>(static_cast<float>(ballDstY - ballSrcY) * t);
        ballY -= static_cast<int>(120.0f * t * (1.0f - t));

        drawBallCentered(renderer, 0, ballX, ballY);
    } else if (captureAnimFrame < landEnd) {
        drawBallCentered(renderer, 1, ballDstX, ballDstY);
    } else if (captureAnimFrame < shakesEnd) {
        int shakeFrame = captureAnimFrame - landEnd;
        int currentShake = shakeFrame / shakeGroupLen;
        int frameInGroup = shakeFrame % shakeGroupLen;

        int wobble = 0;
        if (currentShake < std::min(totalShakes, 4) && frameInGroup < CAPTURE_SHAKE_FRAMES) {
            float shakeT =
                static_cast<float>(frameInGroup) / static_cast<float>(CAPTURE_SHAKE_FRAMES);
            wobble = static_cast<int>(std::sin(shakeT * 6.283f) * 8.0f *
                                      static_cast<float>(PIXEL_SCALE));
        }

        drawBallCentered(renderer, 0, ballDstX + wobble, ballDstY);
    } else {
        if (success) {
            drawBallCentered(renderer, 0, ballDstX, ballDstY);
        } else {
            ui.drawOpponentDaemon(opponentDaemon);
            drawBallCentered(renderer, 2, ballDstX, ballDstY - 10 * PIXEL_SCALE);
        }
    }
}
