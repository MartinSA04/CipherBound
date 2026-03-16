#include "SpriteFont.h"

// Auto-generated glyph lookup table from font.png analysis
// Maps each character to its source rect {srcX, srcY, width, height}

constexpr int START_WIDTH = 8;
constexpr int START_HEIGHT = 2;
constexpr int COLUMN_WIDTH = 16;
constexpr int ROW_HEIGHT = 19;
constexpr int WIDTH = 6;
constexpr int HEIGHT = 16;
constexpr int START_WIDTH_RIGHT = 214;

const std::unordered_map<char, GlyphInfo> SpriteFont::glyphTable = {
    // Uppercase A-J (row 0, left half)
    {'A', {START_WIDTH + 0 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'B', {START_WIDTH + 1 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'C', {START_WIDTH + 2 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'D', {START_WIDTH + 3 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'E', {START_WIDTH + 4 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'F', {START_WIDTH + 5 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'G', {START_WIDTH + 6 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'H', {START_WIDTH + 7 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'I',
     {START_WIDTH + 8 * COLUMN_WIDTH + 1, START_HEIGHT, WIDTH - 2, HEIGHT}},
    {'J', {START_WIDTH + 9 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},

    // Uppercase K-T (row 1, left half)
    {'K',
     {START_WIDTH + 0 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'L',
     {START_WIDTH + 1 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'M',
     {START_WIDTH + 2 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'N',
     {START_WIDTH + 3 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'O',
     {START_WIDTH + 4 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'P',
     {START_WIDTH + 5 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'Q',
     {START_WIDTH + 6 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'R',
     {START_WIDTH + 7 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'S',
     {START_WIDTH + 8 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'T',
     {START_WIDTH + 9 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},

    // Uppercase U-Z (row 2, left half)
    {'U',
     {START_WIDTH + 0 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'V',
     {START_WIDTH + 1 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'W',
     {START_WIDTH + 2 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'X',
     {START_WIDTH + 3 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'Y',
     {START_WIDTH + 4 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'Z',
     {START_WIDTH + 5 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT, WIDTH,
      HEIGHT}},

    // Lowercase a-j (row 0, right half)
    {'a', {START_WIDTH_RIGHT + 0 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'b', {START_WIDTH_RIGHT + 1 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'c', {START_WIDTH_RIGHT + 2 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'d', {START_WIDTH_RIGHT + 3 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'e', {START_WIDTH_RIGHT + 4 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'f',
     {START_WIDTH_RIGHT + 5 * COLUMN_WIDTH, START_HEIGHT, WIDTH - 1, HEIGHT}},
    {'g', {START_WIDTH_RIGHT + 6 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'h', {START_WIDTH_RIGHT + 7 * COLUMN_WIDTH, START_HEIGHT, WIDTH, HEIGHT}},
    {'i',
     {START_WIDTH_RIGHT + 8 * COLUMN_WIDTH + 1, START_HEIGHT, WIDTH - 4,
      HEIGHT}},
    {'j',
     {START_WIDTH_RIGHT + 9 * COLUMN_WIDTH, START_HEIGHT, WIDTH - 2, HEIGHT}},

    // Lowercase k-t (row 1, right half)
    {'k',
     {START_WIDTH_RIGHT + 0 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'l',
     {START_WIDTH_RIGHT + 1 * COLUMN_WIDTH + 1, START_HEIGHT + ROW_HEIGHT,
      WIDTH - 3, HEIGHT}},
    {'m',
     {START_WIDTH_RIGHT + 2 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'n',
     {START_WIDTH_RIGHT + 3 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'o',
     {START_WIDTH_RIGHT + 4 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'p',
     {START_WIDTH_RIGHT + 5 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'q',
     {START_WIDTH_RIGHT + 6 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'r',
     {START_WIDTH_RIGHT + 7 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'s',
     {START_WIDTH_RIGHT + 8 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'t',
     {START_WIDTH_RIGHT + 9 * COLUMN_WIDTH, START_HEIGHT + ROW_HEIGHT,
      WIDTH - 1, HEIGHT}},

    // Lowercase u-z (row 2, right half)
    {'u',
     {START_WIDTH_RIGHT + 0 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT,
      WIDTH, HEIGHT}},
    {'v',
     {START_WIDTH_RIGHT + 1 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT,
      WIDTH, HEIGHT}},
    {'w',
     {START_WIDTH_RIGHT + 2 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT,
      WIDTH, HEIGHT}},
    {'x',
     {START_WIDTH_RIGHT + 3 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT,
      WIDTH, HEIGHT}},
    {'y',
     {START_WIDTH_RIGHT + 4 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT,
      WIDTH, HEIGHT}},
    {'z',
     {START_WIDTH_RIGHT + 5 * COLUMN_WIDTH, START_HEIGHT + 2 * ROW_HEIGHT,
      WIDTH, HEIGHT}},

    // Digits 0-9 (row 3, left half)
    {'0',
     {START_WIDTH + 0 * COLUMN_WIDTH, START_HEIGHT + 3 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'1',
     {START_WIDTH + 1 * COLUMN_WIDTH, START_HEIGHT + 3 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'2',
     {START_WIDTH + 2 * COLUMN_WIDTH, START_HEIGHT + 3 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'3',
     {START_WIDTH + 3 * COLUMN_WIDTH, START_HEIGHT + 3 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'4',
     {START_WIDTH + 4 * COLUMN_WIDTH, START_HEIGHT + 3 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'5',
     {START_WIDTH + 5 * COLUMN_WIDTH, START_HEIGHT + 3 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'6',
     {START_WIDTH + 6 * COLUMN_WIDTH, START_HEIGHT + 3 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'7',
     {START_WIDTH + 7 * COLUMN_WIDTH, START_HEIGHT + 3 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'8',
     {START_WIDTH + 8 * COLUMN_WIDTH, START_HEIGHT + 3 * ROW_HEIGHT, WIDTH,
      HEIGHT}},
    {'9',
     {START_WIDTH + 9 * COLUMN_WIDTH, START_HEIGHT + 3 * ROW_HEIGHT, WIDTH,
      HEIGHT}},

    // Punctuation
    {',', {185, START_HEIGHT, 3, HEIGHT}},
    {'.', {201, START_HEIGHT, 3, HEIGHT}},
    {'-', {200, START_HEIGHT + ROW_HEIGHT, 6, HEIGHT}},
    {':', {206, START_HEIGHT + 3 * ROW_HEIGHT, 3, HEIGHT}},
    {';', {222, START_HEIGHT + 3 * ROW_HEIGHT, 3, HEIGHT}},
    {'!', {239, START_HEIGHT + 3 * ROW_HEIGHT, 2, HEIGHT}},
    {'?', {253, START_HEIGHT + 3 * ROW_HEIGHT, 6, HEIGHT}},
    {'\'', {237, START_HEIGHT + 4 * ROW_HEIGHT, 3, HEIGHT}},
};

void SpriteFont::loadTextures(Renderer &renderer) {
    renderer.loadTexture(FONT_TEXTURE, "assets/sprites/ui/font.png");
    renderer.loadTexture(BNUM_TEXTURE, "assets/sprites/ui/battle_numbers.png");
}

const GlyphInfo *SpriteFont::getGlyph(char c) const {
    auto it = glyphTable.find(c);
    if (it != glyphTable.end()) {
        return &it->second;
    }
    return nullptr;
}

void SpriteFont::getBattleDigitRect(int digit, int &srcX, int &srcY, int &srcW,
                                    int &srcH) const {
    // battle_numbers.png layout:
    // 5 digits per row, 2 rows
    // Each digit: 8px wide x 7px tall
    // Row separator at y=0 and y=8
    // Column separators at x=8, 17, 26, 35
    int col = digit % 5;
    int row = digit / 5;
    srcX = col * 9;     // 9px stride (8px glyph + 1px separator)
    srcY = 1 + row * 8; // rows at y=1 and y=9
    srcW = BNUM_GLYPH_W;
    srcH = BNUM_GLYPH_H;
}

void SpriteFont::drawText(Renderer &renderer, const std::string &text,
                          int screenX, int screenY, int scale,
                          int spacing) const {
    drawTextPartial(renderer, text, text.size(), screenX, screenY, scale,
                    spacing);
}

void SpriteFont::drawTextPartial(Renderer &renderer, const std::string &text,
                                 std::size_t charCount, int screenX,
                                 int screenY, int scale, int spacing,
                                 int maxWidth) const {
    int cursorX = screenX;
    int cursorY = screenY;
    int lineHeight = (FONT_GLYPH_H + 2) * scale;
    std::size_t drawn = 0;

    // If wrapping enabled, break at word boundaries
    if (maxWidth > 0) {
        // Split into words, draw word by word
        std::size_t i = 0;
        while (i < text.size() && drawn < charCount) {
            // Find next word (including leading space)
            std::size_t wordStart = i;
            // Skip leading spaces
            while (i < text.size() && text[i] == ' ')
                ++i;
            // Consume non-space characters
            std::size_t textStart = i;
            while (i < text.size() && text[i] != ' ')
                ++i;

            // Measure the word (without leading spaces for wrap check)
            std::string word = text.substr(textStart, i - textStart);
            int wordWidth = getTextWidth(word, scale, spacing);
            std::size_t spaceCount = textStart - wordStart;
            int spacesWidth = 0;
            for (std::size_t s = 0; s < spaceCount; ++s) {
                spacesWidth += SPACE_WIDTH * scale;
            }

            // Check if word fits on current line
            if (cursorX + spacesWidth + wordWidth > screenX + maxWidth &&
                cursorX > screenX) {
                // Wrap to next line
                cursorX = screenX;
                cursorY += lineHeight;
                // Skip leading spaces at line start
                spaceCount = 0;
                spacesWidth = 0;
            }

            // Draw leading spaces
            for (std::size_t s = 0; s < spaceCount && drawn < charCount; ++s) {
                cursorX += SPACE_WIDTH * scale;
                ++drawn;
            }

            // Draw word characters
            for (char c : word) {
                if (drawn >= charCount)
                    break;
                ++drawn;

                const GlyphInfo *glyph = getGlyph(c);
                if (glyph) {
                    int dstW = glyph->width * scale;
                    int dstH = glyph->height * scale;
                    renderer.drawSpriteRegion(
                        FONT_TEXTURE, glyph->srcX, glyph->srcY, glyph->width,
                        glyph->height, cursorX, cursorY, dstW, dstH);
                    cursorX += (glyph->width + spacing) * scale;
                } else {
                    cursorX += (6 + spacing) * scale;
                }
            }
        }
    } else {
        // No wrapping — original single-line behavior
        for (char c : text) {
            if (drawn >= charCount)
                break;
            ++drawn;

            if (c == ' ') {
                cursorX += SPACE_WIDTH * scale;
                continue;
            }

            const GlyphInfo *glyph = getGlyph(c);
            if (glyph) {
                int dstW = glyph->width * scale;
                int dstH = glyph->height * scale;
                renderer.drawSpriteRegion(
                    FONT_TEXTURE, glyph->srcX, glyph->srcY, glyph->width,
                    glyph->height, cursorX, cursorY, dstW, dstH);
                cursorX += (glyph->width + spacing) * scale;
            } else {
                cursorX += (6 + spacing) * scale;
            }
        }
    }
}

void SpriteFont::drawContinueIndicator(Renderer &renderer, int screenX,
                                       int screenY, int scale) const {
    constexpr int arrowSrcX = 103;
    constexpr int arrowSrcY = 80;
    constexpr int arrowSrcW = 6;
    constexpr int arrowSrcH = 9;

    int dstW = arrowSrcW * scale;
    int dstH = arrowSrcH * scale;

    renderer.drawSpriteRegion(FONT_TEXTURE, arrowSrcX, arrowSrcY, arrowSrcW,
                              arrowSrcH, screenX, screenY, dstW, dstH);
}

void SpriteFont::drawBattleNumber(Renderer &renderer, int number, int screenX,
                                  int screenY, int scale,
                                  bool rightAlign) const {
    std::string numStr = std::to_string(number);
    if (rightAlign)
        screenX -= getBattleNumberWidth(numStr);
    drawBattleNumberString(renderer, numStr, screenX, screenY, scale);
}

void SpriteFont::drawBattleNumberString(Renderer &renderer,
                                        const std::string &text, int screenX,
                                        int screenY, int scale) const {
    int cursorX = screenX;
    int dstW = BNUM_GLYPH_W * scale;
    int dstH = BNUM_GLYPH_H * scale;

    for (char c : text) {
        if (c >= '0' && c <= '9') {
            int digit = c - '0';
            int srcX, srcY, srcW, srcH;
            getBattleDigitRect(digit, srcX, srcY, srcW, srcH);
            renderer.drawSpriteRegion(BNUM_TEXTURE, srcX, srcY, srcW, srcH,
                                      cursorX, screenY, dstW, dstH);
            cursorX += dstW;
        } else if (c == ' ') {
            cursorX += SPACE_WIDTH * scale;
        }
    }
}

int SpriteFont::getTextWidth(const std::string &text, int scale,
                             int spacing) const {
    int width = 0;
    bool first = true;

    for (char c : text) {
        if (!first) {
            width += spacing * scale;
        }
        first = false;

        if (c == ' ') {
            width += SPACE_WIDTH * scale;
            continue;
        }

        const GlyphInfo *glyph = getGlyph(c);
        if (glyph) {
            width += glyph->width * scale;
        } else {
            width += 6 * scale; // default width for unknown chars
        }
    }
    return width;
}

int SpriteFont::getBattleNumberWidth(const std::string &text, int scale) const {
    int width = 0;
    for (char c : text) {
        if (c >= '0' && c <= '9') {
            width += BNUM_GLYPH_W * scale;
        } else if (c == '/') {
            const GlyphInfo *slash = getGlyph('/');
            width += slash ? slash->width * scale : 4 * scale;
        } else if (c == ' ') {
            width += SPACE_WIDTH * scale;
        }
    }
    return width;
}
