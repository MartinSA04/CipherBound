#pragma once
#include "Renderer.h"
#include <cstddef>
#include <string>
#include <unordered_map>

// Describes a single glyph's position in the font spritesheet
struct GlyphInfo {
    int srcX;
    int srcY;
    int width;
    int height;
};

// Renders text using a spritesheet font (font.png) and
// numeric values using battle_numbers.png
class SpriteFont {
  public:
    SpriteFont() = default;

    // Load font textures into the renderer
    void loadTextures(Renderer &renderer);

    // Draw text using font.png spritesheet
    // scale: pixel scale multiplier (e.g. 2 means 2x size)
    // spacing: extra pixels between characters (at source scale)
    void drawText(Renderer &renderer, const std::string &text, int screenX,
                  int screenY, int scale = PIXEL_SCALE, int spacing = 1) const;

    // Draw only the first 'charCount' characters of text (for typewriter
    // effect) If maxWidth > 0, word-wrap text to fit within maxWidth pixels
    void drawTextPartial(Renderer &renderer, const std::string &text,
                         std::size_t charCount, int screenX, int screenY,
                         int scale = PIXEL_SCALE, int spacing = 1,
                         int maxWidth = 0) const;

    // Draw a small flashing triangle indicator at (screenX, screenY)
    void drawContinueIndicator(Renderer &renderer, int screenX, int screenY,
                               int scale = PIXEL_SCALE) const;

    // Draw a number using battle_numbers.png spritesheet
    // Suitable for HP values, levels, etc.
    void drawBattleNumber(Renderer &renderer, int number, int screenX,
                          int screenY, int scale = PIXEL_SCALE,
                          bool rightAlign = false) const;

    // Draw a string of digits and '/' using battle_numbers.png
    // e.g. "35/50" for HP display
    void drawBattleNumberString(Renderer &renderer, const std::string &text,
                                int screenX, int screenY,
                                int scale = PIXEL_SCALE) const;

    // Get the pixel width of a text string at a given scale
    int getTextWidth(const std::string &text, int scale = PIXEL_SCALE,
                     int spacing = 1) const;

    // Get the pixel width of a battle number string at a given scale
    int getBattleNumberWidth(const std::string &text,
                             int scale = PIXEL_SCALE) const;

    // Battle number glyph dimensions (source pixels)
    static constexpr int BNUM_GLYPH_W = 8;
    static constexpr int BNUM_GLYPH_H = 7;

    // Font glyph default height (most glyphs are 12px)
    static constexpr int FONT_GLYPH_H = 12;

    // Space character width in source pixels
    static constexpr int SPACE_WIDTH = 4;

  private:
    // Returns glyph info for a character, or nullptr if not found
    const GlyphInfo *getGlyph(char c) const;

    // Battle number digit source rect (digit 0-9)
    void getBattleDigitRect(int digit, int &srcX, int &srcY, int &srcW,
                            int &srcH) const;

    // Glyph lookup table: char -> source rect in font.png
    static const std::unordered_map<char, GlyphInfo> glyphTable;

    // Texture IDs
    static constexpr const char *FONT_TEXTURE = "ui_font";
    static constexpr const char *BNUM_TEXTURE = "ui_battle_numbers";
};
