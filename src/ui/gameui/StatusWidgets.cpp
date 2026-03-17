#include "../GameUI.h"
#include <algorithm>

void GameUI::drawSelectionArrow(int x, int y, int scale) {
    const int s = scale;
    renderer.drawFilledRect(x, y + 0 * s, 1 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 1 * s, 2 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 2 * s, 3 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 3 * s, 2 * s, 1 * s, TDT4102::Color::black);
    renderer.drawFilledRect(x, y + 4 * s, 1 * s, 1 * s, TDT4102::Color::black);
}

void GameUI::drawSpriteHPBar(int x, int y, int width, int currentHP, int maxHP, int scale) {
    const float ratio =
        (maxHP > 0) ? static_cast<float>(currentHP) / static_cast<float>(maxHP) : 0.0f;
    const int filledWidth = static_cast<int>(static_cast<float>(width) * ratio);

    const int emptySrcY = 9;
    int fillSrcY = 6;
    if (ratio > 0.5f)
        fillSrcY = 0;
    else if (ratio > 0.2f)
        fillSrcY = 3;

    const int barH = 3 * scale;

    for (int dx = 0; dx < width; dx += scale) {
        const int tileW = std::min(scale, width - dx);
        renderer.drawSpriteRegion("ui_hp_bar", 0, emptySrcY, 1, 3, x + dx, y, tileW, barH);
    }

    if (filledWidth <= 0)
        return;

    for (int dx = 0; dx < filledWidth; dx += scale) {
        const int tileW = std::min(scale, filledWidth - dx);
        renderer.drawSpriteRegion("ui_hp_bar", 0, fillSrcY, 1, 3, x + dx, y, tileW, barH);
    }
}

void GameUI::drawSpriteEXPBar(int x, int y, int width, int currentEXP, int maxEXP, int scale) {
    int filledWidth = 0;
    if (maxEXP > 0)
        filledWidth = std::min(width, currentEXP * width / maxEXP);

    const int barH = 2 * scale;

    for (int dx = 0; dx < width; dx += scale) {
        const int tileW = std::min(scale, width - dx);
        renderer.drawSpriteRegion("ui_exp_bar", 0, 0, 1, 2, x + dx, y, tileW, barH);
    }

    if (filledWidth <= 0)
        return;

    for (int dx = 0; dx < filledWidth; dx += scale) {
        const int tileW = std::min(scale, filledWidth - dx);
        renderer.drawSpriteRegion("ui_exp_bar", 0, 2, 1, 2, x + dx, y, tileW, barH);
    }
}
