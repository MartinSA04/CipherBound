#include "../GameUI.h"
#include <algorithm>

int GameUI::getDialogueTextMaxWidth(int scale) const {
    return (dialogueTextBarWidth - dialogueTextPaddingX * 2) * scale;
}

std::vector<std::string> GameUI::paginateDialogueText(const std::string &text) const {
    const auto wrapped = spriteFont.wrapText(text, PIXEL_SCALE, 1, getDialogueTextMaxWidth());
    std::vector<std::string> sections;

    for (std::size_t i = 0; i < wrapped.size(); i += dialogueTextMaxLines) {
        std::string section = wrapped[i];
        const std::size_t sectionEnd =
            std::min(i + static_cast<std::size_t>(dialogueTextMaxLines), wrapped.size());
        for (std::size_t j = i + 1; j < sectionEnd; ++j) {
            section += '\n';
            section += wrapped[j];
        }
        sections.push_back(std::move(section));
    }

    if (sections.empty())
        sections.push_back("");

    return sections;
}

void GameUI::resetTypewriterForCurrentSection() {
    if (typewriterSections.empty()) {
        typewriterFullText.clear();
    } else {
        typewriterFullText = typewriterSections[typewriterSectionIndex];
    }

    typewriterCharsRevealed = 0;
    typewriterFrameCounter = 0;
    typewriterIndicatorTimer = 0;
}

bool GameUI::advanceTextSection() {
    if (typewriterSectionIndex + 1 >= typewriterSections.size())
        return false;

    ++typewriterSectionIndex;
    resetTypewriterForCurrentSection();
    return true;
}

void GameUI::setDialogueText(const std::string &text) {
    if (text == typewriterSourceText)
        return;

    typewriterSourceText = text;
    typewriterSections = paginateDialogueText(text);
    typewriterSectionIndex = 0;
    resetTypewriterForCurrentSection();
}

bool GameUI::updateTypewriter(const bool inputConfirm) {
    if (isTextFullyRevealed()) {
        typewriterIndicatorTimer++;
        if (!inputConfirm)
            return false;

        if (advanceTextSection())
            return false;
        return true;
    }

    bool fast = input.isConfirmHeld() || input.isCancelHeld();
    int speed = fast ? typewriterFastSpeed : typewriterSpeed;

    typewriterFrameCounter++;
    if (typewriterFrameCounter >= speed) {
        typewriterFrameCounter = 0;
        typewriterCharsRevealed++;
        if (typewriterCharsRevealed >= typewriterFullText.size()) {
            typewriterCharsRevealed = typewriterFullText.size();
            typewriterIndicatorTimer = 0;
        }
    }
    if (!inputConfirm)
        return false;

    if (!isTextFullyRevealed()) {
        revealAllText();
        return false;
    }
    return true;
}

bool GameUI::isTextFullyRevealed() const {
    return typewriterCharsRevealed >= typewriterFullText.size();
}

void GameUI::revealAllText() {
    typewriterCharsRevealed = typewriterFullText.size();
    typewriterIndicatorTimer = 0;
}

void GameUI::startDialogue(const std::string &speaker, const std::vector<std::string> &lines) {
    dialogueSpeaker = speaker;
    dialogueLines.clear();
    for (const auto &line : lines) {
        const auto paginatedLine = paginateDialogueText(line);
        dialogueLines.insert(dialogueLines.end(), paginatedLine.begin(), paginatedLine.end());
    }
    dialogueLineIndex = 0;
    if (!dialogueLines.empty())
        setDialogueText(dialogueLines[0]);
}

bool GameUI::advanceDialogueLine() {
    dialogueLineIndex++;
    if (isDialogueActive()) {
        setDialogueText(dialogueLines[static_cast<std::size_t>(dialogueLineIndex)]);
        return true;
    }
    return false;
}

bool GameUI::isDialogueActive() const {
    return dialogueLineIndex >= 0 && dialogueLineIndex < static_cast<int>(dialogueLines.size());
}

const std::string &GameUI::getCurrentDialogueLine() const {
    return dialogueLines[static_cast<std::size_t>(dialogueLineIndex)];
}

const std::string &GameUI::getDialogueSpeaker() const { return dialogueSpeaker; }

void GameUI::drawDialogueBox(const std::string &speaker, const std::string &text) {
    setDialogueText(text);

    int panelY = WINDOW_HEIGHT - UI_PANEL_HEIGHT;
    int scale = PIXEL_SCALE;

    drawTextBar(panelY);

    int textBarW = dialogueTextBarWidth * scale;
    int textBarH = dialogueTextBarHeight * scale;
    int textBarX = (WINDOW_WIDTH - textBarW) / 2;
    int textBarY = panelY + (UI_PANEL_HEIGHT - textBarH) / 2;

    int textX = textBarX + dialogueTextPaddingX * scale;
    int textY = textBarY + 6 * scale;

    const int textMaxW = getDialogueTextMaxWidth(scale);
    spriteFont.drawTextPartial(renderer, typewriterFullText, typewriterCharsRevealed, textX, textY,
                               scale, 1, textMaxW);

    if (isTextFullyRevealed()) {
        int bouncePhase = typewriterIndicatorTimer % 20;
        int bounceOffset = (bouncePhase < 10) ? (bouncePhase / 3) : (3 - (bouncePhase - 10) / 3);

        int indicatorX = textBarX + textBarW - 12 * scale;
        int indicatorY = textBarY + textBarH - 14 * scale + bounceOffset * scale;

        spriteFont.drawContinueIndicator(renderer, indicatorX, indicatorY, scale);
    }

    if (!speaker.empty()) {
        int spkW = spriteFont.getTextWidth(speaker, scale) + 6 * scale;
        int spkH = 16 * scale + 4 * scale;
        int spkX = textBarX + 4 * scale;
        int spkY = textBarY - spkH - 2;

        renderer.drawFilledRect(spkX, spkY, spkW, spkH, TDT4102::Color::white);
        renderer.drawRect(spkX, spkY, spkW, spkH, TDT4102::Color::transparent,
                          TDT4102::Color::black);
        spriteFont.drawText(renderer, speaker, spkX + 3 * scale, spkY + 2 * scale, scale);
    }
}

void GameUI::drawChoiceBox(const std::vector<std::string> &options, int selected) {
    const int scale = PIXEL_SCALE;
    const int innerPaddingX = 5 * scale;
    const int innerPaddingY = 4 * scale;
    const int outerPadding = 5 * scale;
    const int gapAboveTextbox = 3 * scale;
    int maxTextW = 0;
    for (const auto &opt : options) {
        int tw = spriteFont.getTextWidth(opt, scale);
        if (tw > maxTextW)
            maxTextW = tw;
    }
    const int boxWidth = maxTextW + innerPaddingX * 2;
    const int itemHeight = 16 * scale + 4 * scale;
    int boxHeight = innerPaddingY * 2;
    const int optionCount = static_cast<int>(options.size());
    for (int i = 0; i < optionCount; ++i)
        boxHeight += itemHeight;

    const int boxX = WINDOW_WIDTH - boxWidth - outerPadding;
    const int boxY = WINDOW_HEIGHT - UI_PANEL_HEIGHT - boxHeight - gapAboveTextbox;

    renderer.drawFilledRect(boxX, boxY, boxWidth, boxHeight, TDT4102::Color{240, 245, 255});
    renderer.drawRect(boxX, boxY, boxWidth, boxHeight, TDT4102::Color::transparent,
                      TDT4102::Color{60, 70, 100});

    int oy = boxY + innerPaddingY;
    for (int i = 0; i < optionCount; ++i) {
        if (i == selected)
            drawSelectionArrow(boxX + 2 * scale, oy + 4 * scale, scale);
        spriteFont.drawText(renderer, options[static_cast<std::size_t>(i)], boxX + 6 * scale, oy,
                            scale);
        oy += itemHeight;
    }
}

void GameUI::drawShopQuantityBox(int quantity) {
    const int scale = PIXEL_SCALE;
    const int innerPaddingX = 5 * scale;
    const int innerPaddingY = 4 * scale;
    const int outerPadding = 5 * scale;
    const int gapAboveTextbox = 3 * scale;
    const std::string quantityText = "x" + std::to_string(quantity);

    const int boxWidth = spriteFont.getTextWidth(quantityText, scale) + innerPaddingX * 2;
    const int boxHeight = 16 * scale + innerPaddingY * 2;
    const int boxX = WINDOW_WIDTH - boxWidth - outerPadding;
    const int boxY = WINDOW_HEIGHT - UI_PANEL_HEIGHT - boxHeight - gapAboveTextbox;

    renderer.drawFilledRect(boxX, boxY, boxWidth, boxHeight, TDT4102::Color{240, 245, 255});
    renderer.drawRect(boxX, boxY, boxWidth, boxHeight, TDT4102::Color::transparent,
                      TDT4102::Color{60, 70, 100});
    spriteFont.drawText(renderer, quantityText, boxX + innerPaddingX, boxY + innerPaddingY, scale);
}

void GameUI::drawTextBox(int x, int y, int width, int height, const std::string &text) {
    renderer.drawFilledRect(x, y, width, height, TDT4102::Color::white);
    renderer.drawRect(x, y, width, height, TDT4102::Color::transparent, TDT4102::Color::black);
    spriteFont.drawText(renderer, text, x + 16, y + 14, PIXEL_SCALE);
}

void GameUI::drawTextBar(int panelY) {
    int scale = PIXEL_SCALE;
    int barW = dialogueTextBarWidth * scale;
    int barH = dialogueTextBarHeight * scale;
    int barX = (WINDOW_WIDTH - barW) / 2;
    int barY = panelY + (UI_PANEL_HEIGHT - barH) / 2;
    renderer.drawSpriteRaw("ui_text_bar", barX, barY, barW, barH);
}

void GameUI::drawNarrowTextBar(int x, int y, int srcW, int scale) {
    constexpr int fullSrcW = 252;
    constexpr int srcH = 46;
    constexpr int edgeW = 8;

    int clampedSrcW = std::max(srcW, edgeW * 2 + 1);
    if (clampedSrcW > fullSrcW)
        clampedSrcW = fullSrcW;

    int dstH = srcH * scale;
    int middleSrcW = clampedSrcW - edgeW * 2;

    renderer.drawSpriteRegion("ui_text_bar", 0, 0, edgeW, srcH, x, y, edgeW * scale, dstH);

    int midSrcX = fullSrcW / 2 - middleSrcW / 2;
    renderer.drawSpriteRegion("ui_text_bar", midSrcX, 0, middleSrcW, srcH, x + edgeW * scale, y,
                              middleSrcW * scale, dstH);

    renderer.drawSpriteRegion("ui_text_bar", fullSrcW - edgeW, 0, edgeW, srcH,
                              x + (edgeW + middleSrcW) * scale, y, edgeW * scale, dstH);
}
