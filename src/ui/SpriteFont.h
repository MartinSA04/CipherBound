#pragma once
#include "Renderer.h"
#include <cstddef>
#include <string>
#include <unordered_map>

struct GlyphInfo {
    int srcX;
    int srcY;
    int width;
    int height;
};

class SpriteFont {
  public:
    SpriteFont() = default;

    void loadTextures(Renderer &renderer);

    void drawText(Renderer &renderer, const std::string &text, int screenX, int screenY,
                  int scale = PIXEL_SCALE, int spacing = 1) const;

    void drawTextPartial(Renderer &renderer, const std::string &text, std::size_t charCount,
                         int screenX, int screenY, int scale = PIXEL_SCALE, int spacing = 1,
                         int maxWidth = 0) const;

    void drawContinueIndicator(Renderer &renderer, int screenX, int screenY,
                               int scale = PIXEL_SCALE) const;

    void drawBattleNumber(Renderer &renderer, int number, int screenX, int screenY,
                          int scale = PIXEL_SCALE, bool rightAlign = false) const;

    void drawBattleNumberString(Renderer &renderer, const std::string &text, int screenX,
                                int screenY, int scale = PIXEL_SCALE) const;

    int getTextWidth(const std::string &text, int scale = PIXEL_SCALE, int spacing = 1) const;

    int getBattleNumberWidth(const std::string &text, int scale = PIXEL_SCALE) const;

    static constexpr int BNUM_GLYPH_W = 8;
    static constexpr int BNUM_GLYPH_H = 7;
    static constexpr int FONT_GLYPH_H = 12;
    static constexpr int SPACE_WIDTH = 4;

  private:
    const GlyphInfo *getGlyph(char c) const;

    void drawTextChar(Renderer &renderer, char c, int &cursorX, int cursorY, int scale, int spacing,
                      std::size_t &drawn) const;

    void getBattleDigitRect(int digit, int &srcX, int &srcY, int &srcW, int &srcH) const;

    static const std::unordered_map<char, GlyphInfo> glyphTable;

    static constexpr const char *FONT_TEXTURE = "ui_font";
    static constexpr const char *BNUM_TEXTURE = "ui_battle_numbers";
};
