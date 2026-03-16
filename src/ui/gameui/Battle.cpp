#include "../GameUI.h"
#include <algorithm>

bool GameUI::tickHPAnimation(int targetPlayerHP, int targetOpponentHP, int maxPlayerHP,
                             int maxOpponentHP) {
    int playerStep = std::max(1, maxPlayerHP / 15);
    int opponentStep = std::max(1, maxOpponentHP / 15);

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

EXPTickResult GameUI::tickEXPAnimation(int targetEXP, int expNeeded) {
    static constexpr int EXP_ANIM_FRAMES = 120;
    int destination = std::min(targetEXP, expNeeded);

    if (expAnimStartEXP < 0)
        expAnimStartEXP = playerDisplayEXP;

    expAnimFrame++;

    if (expAnimFrame >= EXP_ANIM_FRAMES) {
        playerDisplayEXP = destination;
        expAnimFrame = 0;
        expAnimStartEXP = -1;

        if (playerDisplayEXP >= expNeeded)
            return EXPTickResult::filledBar;
        return EXPTickResult::reachedTarget;
    }

    playerDisplayEXP =
        expAnimStartEXP + (destination - expAnimStartEXP) * (expAnimFrame / EXP_ANIM_FRAMES);

    if (playerDisplayEXP >= expNeeded) {
        playerDisplayEXP = expNeeded;
        expAnimFrame = 0;
        expAnimStartEXP = -1;
        return EXPTickResult::filledBar;
    }

    return EXPTickResult::inProgress;
}

void GameUI::drawSelectionArrow(int x, int y, int scale) {
    int s = scale;
    renderer.drawFilledRect(x, y + 0 * s, 1 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 1 * s, 2 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 2 * s, 3 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 3 * s, 2 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 4 * s, 1 * s, 1 * s, TDT4102::Color::black);
}

void GameUI::drawOpponentInfoBar(const Daemon *opponentDaemon, int offsetX) {
    const int scale = PIXEL_SCALE;

    int oppPanelW = 122 * scale;
    int oppPanelH = 35 * scale;
    int oppPanelX = offsetX;
    int oppPanelY = 16;

    renderer.drawSpriteRaw("ui_opponent_info", oppPanelX, oppPanelY, oppPanelW, oppPanelH);

    if (opponentDaemon) {
        int oppNameX = oppPanelX + 2 * scale;
        int oppNameY = oppPanelY + 9 * scale;
        spriteFont.drawText(renderer, opponentDaemon->getNickname(), oppNameX, oppNameY, scale);

        int oppLvlX = oppPanelX + 83 * scale;
        int oppLvlY = oppPanelY + 12 * scale;
        spriteFont.drawBattleNumber(renderer, opponentDaemon->getLevel(), oppLvlX, oppLvlY, scale);

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

    renderer.drawSpriteRaw("ui_player_info", plyPanelX, plyPanelY, plyPanelW, plyPanelH);

    if (playerDaemon) {
        int plyNameX = plyPanelX + 18 * scale;
        int plyNameY = plyPanelY + 10 * scale;
        spriteFont.drawText(renderer, playerDaemon->getNickname(), plyNameX, plyNameY, scale);

        int plyLvlX = plyPanelX + 105 * scale;
        int plyLvlY = plyPanelY + 12 * scale;
        spriteFont.drawBattleNumber(renderer, playerDaemon->getLevel(), plyLvlX, plyLvlY, scale);

        int plyHPBarX = plyPanelX + 72 * scale;
        int plyHPBarY = plyPanelY + 25 * scale;
        int plyHPBarW = 48 * scale;
        drawSpriteHPBar(plyHPBarX, plyHPBarY, plyHPBarW, playerDisplayHP, playerDaemon->getMaxHP(),
                        scale);

        int hpNumX = plyPanelX + 92 * scale;
        int hpNumY = plyPanelY + 32 * scale;
        int hpPad = 3 * scale;
        spriteFont.drawBattleNumber(renderer, playerDisplayHP, hpNumX - hpPad, hpNumY, scale, true);
        spriteFont.drawBattleNumber(renderer, playerDaemon->getMaxHP(), hpNumX + hpPad, hpNumY,
                                    scale);

        int expBarX = plyPanelX + 24 * scale;
        int expBarY = plyPanelY + 42 * scale;
        int expBarW = 96 * scale;
        int expNeeded = playerDaemon->getExpNeeded();
        drawSpriteEXPBar(expBarX, expBarY, expBarW, playerDisplayEXP, expNeeded, scale);
    }
}

void GameUI::drawOpponentDaemon(const Daemon *opponentDaemon, int offsetX, int offsetY) {
    int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getOpponentBaseGeometry();

    if (opponentDaemon) {
        const std::string &opponentSpeciesName = opponentDaemon->getSpecies().name;
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

void GameUI::drawPlayerDaemon(const Daemon *playerDaemon, int offsetX, int offsetY) {
    const int scale = PIXEL_SCALE;
    auto [baseX, baseY, baseW, baseH] = getPlayerBaseGeometry();

    if (playerDaemon) {
        const std::string &playerSpeciesName = playerDaemon->getSpecies().name;
        loadDaemonSprite(playerSpeciesName);
        std::string plySpriteId = "daemon_" + playerDaemon->getSpecies().name + "_back";
        TDT4102::Image &plyImg = renderer.getTexture(plySpriteId);
        int plySprW = plyImg.width * scale;
        int plySprH = plyImg.height * scale;
        int plySprX = baseX + baseW / 2 - plySprW / 2 + offsetX;
        int plySprY = baseY - plySprH + baseH + offsetY;
        renderer.drawSpriteRaw(plySpriteId, plySprX, plySprY, plySprW, plySprH);
    }
}

void GameUI::drawPlayerBackSprite(int x, int y, int dstW, int dstH, int frame) {
    constexpr int FRAME_W = 80;
    constexpr int FRAME_H = 80;
    constexpr int COLS = 3;
    constexpr int TOTAL_FRAMES = 5;
    int f = frame % TOTAL_FRAMES;
    int srcX = (f % COLS) * FRAME_W;
    int srcY = (f / COLS) * FRAME_H;
    renderer.drawSpriteRegion("player_back", srcX, srcY, FRAME_W, FRAME_H, x, y, dstW, dstH);
}

void GameUI::drawBattleBackground() {
    int battleH = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, battleH, TDT4102::Color{200, 220, 200});
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

    int trainerW = 40 * scale;
    int trainerH = 56 * scale;
    int trainerX = baseX + baseW / 2 - trainerW / 2 + offsetX;
    int trainerY = baseY + baseH - trainerH - 10 * scale;
    renderer.drawFilledRect(trainerX, trainerY, trainerW, trainerH, TDT4102::Color{60, 60, 80});
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
    int playerSlideOut = static_cast<int>(-WINDOW_WIDTH * t);
    int throwFrame = battleIntroFrame / 4;
    if (t < 0.5f)
        drawPlayerBackOnBase(playerSlideOut, throwFrame);

    int daemonSlideIn = static_cast<int>(-WINDOW_WIDTH * (1.0f - t));
    drawPlayerDaemon(playerDaemon, daemonSlideIn);

    int infoSlideIn = static_cast<int>(WINDOW_WIDTH * (1.0f - t));
    drawPlayerInfoBar(playerDaemon, infoSlideIn);
}

void GameUI::drawSpriteHPBar(int x, int y, int width, int currentHP, int maxHP, int scale) {
    float ratio = (maxHP > 0) ? static_cast<float>(currentHP) / static_cast<float>(maxHP) : 0.0f;
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
        renderer.drawSpriteRegion("ui_hp_bar", 0, emptySrcY, 1, 3, x + dx, y, tileW, barH);
    }

    if (filledWidth > 0) {
        for (int dx = 0; dx < filledWidth; dx += scale) {
            int tileW = std::min(scale, filledWidth - dx);
            renderer.drawSpriteRegion("ui_hp_bar", 0, fillSrcY, 1, 3, x + dx, y, tileW, barH);
        }
    }
}

void GameUI::drawSpriteEXPBar(int x, int y, int width, int currentEXP, int maxEXP, int scale) {
    int filledWidth = 0;
    if (maxEXP > 0)
        filledWidth = std::min(width, currentEXP * width / maxEXP);

    int barH = 2 * scale;

    for (int dx = 0; dx < width; dx += scale) {
        int tileW = std::min(scale, width - dx);
        renderer.drawSpriteRegion("ui_exp_bar", 0, 0, 1, 2, x + dx, y, tileW, barH);
    }

    if (filledWidth > 0) {
        for (int dx = 0; dx < filledWidth; dx += scale) {
            int tileW = std::min(scale, filledWidth - dx);
            renderer.drawSpriteRegion("ui_exp_bar", 0, 2, 1, 2, x + dx, y, tileW, barH);
        }
    }
}
