#include "NameEntryPanel.h"
#include "../../ui/Renderer.h"
#include "../../ui/SpriteFont.h"
#include "../../ui/UiConstants.h"
#include <array>
#include <string_view>
#include <utility>

namespace {

constexpr int nameGridColumns = 6;
constexpr int firstSpecialKeyIndex = 26;
constexpr int spaceKeyIndex = 26;
constexpr int deleteKeyIndex = 27;
constexpr int auxiliaryKeyIndex = 28;
constexpr int doneKeyIndex = 29;
constexpr int totalKeyCount = 30;

constexpr int nameBoxWidth = 520;
constexpr int nameBoxHeight = 72;
constexpr int nameBoxTextX = 18;
constexpr int nameBoxTextY = 38;
constexpr int countTextOffsetX = 55;

constexpr int cellWidth = 92;
constexpr int cellHeight = 48;
constexpr int cellGap = 8;
constexpr int gridOffsetY = 100;

constexpr std::array<std::string_view, firstSpecialKeyIndex> letterKeys{
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
    "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
};

std::string trimTrailingSpaces(std::string value) {
    while (!value.empty() && value.back() == ' ')
        value.pop_back();
    return value;
}

std::string displayText(const std::string &value, int maxLength) {
    if (value.empty())
        return "-";
    if (value.size() >= static_cast<std::size_t>(maxLength))
        return value;
    return value + "-";
}

} // namespace

NameEntryPanel::NameEntryPanel(std::string auxiliaryKeyLabel, int maxLength)
    : auxiliaryKeyLabel(std::move(auxiliaryKeyLabel)), maxLength(maxLength) {}

void NameEntryPanel::reset(std::string initialText) {
    text = std::move(initialText);
    selectedKey = 0;
}

void NameEntryPanel::setAuxiliaryKeyLabel(std::string label) {
    auxiliaryKeyLabel = std::move(label);
}

void NameEntryPanel::setText(std::string value) { text = std::move(value); }

const std::string &NameEntryPanel::getText() const { return text; }

std::string NameEntryPanel::normalizedText() const { return trimTrailingSpaces(text); }

std::string NameEntryPanel::normalizedTextOr(const std::string &fallback) const {
    const std::string normalized = normalizedText();
    return normalized.empty() ? fallback : normalized;
}

void NameEntryPanel::navigate(const InputManager &input) {
    int row = selectedKey / nameGridColumns;
    int col = selectedKey % nameGridColumns;

    if (input.isLeftPressed()) {
        col = (col + nameGridColumns - 1) % nameGridColumns;
    } else if (input.isRightPressed()) {
        col = (col + 1) % nameGridColumns;
    } else if (input.isUpPressed()) {
        row = (row + totalKeyCount / nameGridColumns - 1) % (totalKeyCount / nameGridColumns);
    } else if (input.isDownPressed()) {
        row = (row + 1) % (totalKeyCount / nameGridColumns);
    }

    selectedKey = row * nameGridColumns + col;
}

NameEntryAction NameEntryPanel::backspace() {
    if (text.empty())
        return NameEntryAction::none;

    text.pop_back();
    return NameEntryAction::edited;
}

NameEntryAction NameEntryPanel::activateSelectedKey() {
    if (selectedKey < firstSpecialKeyIndex) {
        return appendCharacter(letterKeys[static_cast<std::size_t>(selectedKey)].front())
                   ? NameEntryAction::edited
                   : NameEntryAction::none;
    }

    if (selectedKey == spaceKeyIndex)
        return appendSpace() ? NameEntryAction::edited : NameEntryAction::none;
    if (selectedKey == deleteKeyIndex)
        return backspace();
    if (selectedKey == auxiliaryKeyIndex)
        return NameEntryAction::auxiliary;
    if (selectedKey == doneKeyIndex)
        return NameEntryAction::submit;
    return NameEntryAction::none;
}

void NameEntryPanel::render(GameUI &ui, const NameEntryRenderOptions &options) const {
    Renderer &renderer = ui.getRenderer();
    SpriteFont &spriteFont = ui.getSpriteFont();

    const int nameBoxX = (WINDOW_WIDTH - nameBoxWidth) / 2;
    renderer.drawFilledRect(nameBoxX, options.nameBoxY, nameBoxWidth, nameBoxHeight,
                            TDT4102::Color{228, 236, 255});
    renderer.drawRect(nameBoxX, options.nameBoxY, nameBoxWidth, nameBoxHeight,
                      TDT4102::Color::transparent, TDT4102::Color{24, 32, 74});
    renderer.drawText(options.fieldLabel, nameBoxX + nameBoxTextX, options.nameBoxY + 12,
                      TDT4102::Color{20, 30, 70}, 16);

    const std::string shownText = displayText(text, maxLength);
    renderer.drawText(shownText, nameBoxX + nameBoxTextX, options.nameBoxY + nameBoxTextY,
                      TDT4102::Color{20, 30, 70}, 28);

    const std::string countText =
        std::to_string(text.size()) + "/" + std::to_string(maxLength);
    renderer.drawText(countText, nameBoxX + nameBoxWidth - countTextOffsetX, options.nameBoxY + 18,
                      TDT4102::Color{70, 82, 120}, 14);

    const int gridWidth = nameGridColumns * cellWidth + (nameGridColumns - 1) * cellGap;
    const int gridX = (WINDOW_WIDTH - gridWidth) / 2;
    const int gridY = options.nameBoxY + gridOffsetY;

    for (int i = 0; i < totalKeyCount; ++i) {
        const int row = i / nameGridColumns;
        const int col = i % nameGridColumns;
        const int x = gridX + col * (cellWidth + cellGap);
        const int y = gridY + row * (cellHeight + cellGap);
        const bool isSelected = i == selectedKey;

        const TDT4102::Color fill =
            isSelected ? TDT4102::Color{228, 236, 255} : TDT4102::Color{35, 35, 80};
        const TDT4102::Color border =
            isSelected ? TDT4102::Color{24, 32, 74} : TDT4102::Color{80, 80, 130};
        renderer.drawFilledRect(x, y, cellWidth, cellHeight, fill);
        renderer.drawRect(x, y, cellWidth, cellHeight, TDT4102::Color::transparent, border);

        const std::string label = keyLabel(i);
        const int labelScale = label.size() > 1 ? 1 : 2;
        const int labelWidth = spriteFont.getTextWidth(label, labelScale);
        const int labelX = x + (cellWidth - labelWidth) / 2;
        const int labelY = y + (cellHeight - 8 * labelScale) / 2 - 2;
        spriteFont.drawText(renderer, label, labelX, labelY, labelScale, 1);
    }

    renderer.drawText(options.footerPrimary, WINDOW_WIDTH / 2 - 170, WINDOW_HEIGHT - 58,
                      TDT4102::Color{190, 196, 220}, 13);
    renderer.drawText(options.footerSecondary, WINDOW_WIDTH / 2 - 208, WINDOW_HEIGHT - 34,
                      TDT4102::Color{120, 130, 175}, 12);
}

std::string NameEntryPanel::keyLabel(int index) const {
    if (index < firstSpecialKeyIndex)
        return std::string(letterKeys[static_cast<std::size_t>(index)]);
    if (index == spaceKeyIndex)
        return "SPACE";
    if (index == deleteKeyIndex)
        return "DEL";
    if (index == auxiliaryKeyIndex)
        return auxiliaryKeyLabel;
    return "DONE";
}

bool NameEntryPanel::appendCharacter(char c) {
    if (text.size() >= static_cast<std::size_t>(maxLength))
        return false;

    text.push_back(c);
    return true;
}

bool NameEntryPanel::appendSpace() {
    if (text.empty() || text.size() >= static_cast<std::size_t>(maxLength) || text.back() == ' ')
        return false;

    text.push_back(' ');
    return true;
}
