#include "TitleScreenMode.h"
#include "../../audio/MusicManager.h"
#include "../../save/SaveManager.h"
#include "../../state/World.h"
#include "../../state/player/Player.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"
#include "../../ui/Renderer.h"
#include "../../ui/SpriteFont.h"
#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

namespace {

constexpr int startingMoney = 3000;
constexpr int potionId = 1;
constexpr int daemonBallId = 5;
constexpr int maxPlayerNameLength = 12;
constexpr int nameGridColumns = 6;

constexpr std::array<std::string_view, 30> nameKeys{
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",     "M",   "N",    "O",
    "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "SPACE", "DEL", "BACK", "DONE",
};

void moveNameSelection(int &selected, const InputManager &input) {
    int row = selected / nameGridColumns;
    int col = selected % nameGridColumns;

    if (input.isLeftPressed()) {
        col = (col + nameGridColumns - 1) % nameGridColumns;
    } else if (input.isRightPressed()) {
        col = (col + 1) % nameGridColumns;
    } else if (input.isUpPressed()) {
        row = (row + static_cast<int>(nameKeys.size() / nameGridColumns) - 1) %
              static_cast<int>(nameKeys.size() / nameGridColumns);
    } else if (input.isDownPressed()) {
        row = (row + 1) % static_cast<int>(nameKeys.size() / nameGridColumns);
    }

    selected = row * nameGridColumns + col;
}

std::string normalizedPlayerName(std::string name) {
    while (!name.empty() && name.back() == ' ')
        name.pop_back();
    return name;
}

bool appendNameKey(std::string &name, std::string_view key) {
    if (key == "SPACE") {
        if (name.empty() || name.size() >= static_cast<std::size_t>(maxPlayerNameLength) ||
            name.back() == ' ') {
            return false;
        }
        name.push_back(' ');
        return true;
    }

    if (key.size() != 1 || name.size() >= static_cast<std::size_t>(maxPlayerNameLength))
        return false;

    name.push_back(key.front());
    return true;
}

std::string displayPlayerName(const std::string &name) {
    if (name.empty())
        return "-";
    if (name.size() >= static_cast<std::size_t>(maxPlayerNameLength))
        return name;
    return name + "-";
}

void startNewGame(GameContext &ctx, std::string playerName) {
    ctx.world.generate(ctx.pokedex);

    Player &player = ctx.world.getPlayer();
    player.setName(std::move(playerName));
    player.setMoney(startingMoney);
    player.addItem(potionId, 5);
    player.addItem(daemonBallId, 10);

    ctx.pushRequest(ModeRequest::dialogue(
        "", {"What a good night sleep!", "Its finally time to start my degree!",
             "I will pursue my fathers great dream.", "To understand what Daemons actually are!",
             "If only he would be alive to see it...",
             "I should see if my schedule has been delivered in the mail."}));

    const MusicTrack mapTrack = MusicManager::trackForMap(ctx.world.getCurrentMapId());
    ctx.music.play(mapTrack, ctx.ui.getRenderer().getWindow());
}

} // namespace

void TitleScreenMode::onEnter(GameContext &ctx) {
    slotInfos = ctx.saveManager.getSlotInfos();
    selected = 0;
    phase = Phase::titleCard;
    titleTimer = 0;
    nameKeySelected = 0;
    pendingName.clear();
}

void TitleScreenMode::update(GameContext &ctx, InputManager &input) {
    switch (phase) {
    case Phase::titleCard: {
        titleTimer++;
        // Wait at least 15 frames before allowing skip, then any key proceeds
        if (titleTimer > 15 && (input.isConfirmPressed() || input.isCancelPressed())) {
            phase = Phase::saveSlotSelect;
        }
        break;
    }

    case Phase::saveSlotSelect: {
        ctx.ui.navigateVertical(selected, ctx.saveManager.getSlotCount());

        if (input.isConfirmPressed()) {
            ctx.playSound(SoundEffect::select);
            ctx.flow.currentSaveSlot = selected;
            const auto &info = slotInfos[static_cast<std::size_t>(selected)];

            if (info.exists) {
                ctx.saveManager.loadGame(ctx.saveManager.getSavePath(selected),
                                         ctx.world.getPlayer(), ctx.world, ctx.pokedex);
                ctx.pushRequest(ModeRequest::changeState(GameState::overworld));

                const MusicTrack mapTrack = MusicManager::trackForMap(ctx.world.getCurrentMapId());
                ctx.music.play(mapTrack, ctx.ui.getRenderer().getWindow());
            } else {
                pendingName.clear();
                nameKeySelected = 0;
                phase = Phase::nameEntry;
            }
        }
        break;
    }

    case Phase::nameEntry: {
        moveNameSelection(nameKeySelected, input);

        if (input.isCancelPressed()) {
            ctx.playSound(SoundEffect::select);
            if (!pendingName.empty()) {
                pendingName.pop_back();
            } else {
                phase = Phase::saveSlotSelect;
            }
            break;
        }

        if (!input.isConfirmPressed())
            break;

        const std::string_view key = nameKeys[static_cast<std::size_t>(nameKeySelected)];

        if (key == "DEL") {
            if (!pendingName.empty()) {
                pendingName.pop_back();
                ctx.playSound(SoundEffect::select);
            }
            break;
        }

        if (key == "BACK") {
            ctx.playSound(SoundEffect::select);
            phase = Phase::saveSlotSelect;
            pendingName.clear();
            nameKeySelected = 0;
            break;
        }

        if (key == "DONE") {
            const std::string finalName = normalizedPlayerName(pendingName);
            if (!finalName.empty()) {
                ctx.playSound(SoundEffect::select);
                startNewGame(ctx, finalName);
            }
            break;
        }

        if (appendNameKey(pendingName, key))
            ctx.playSound(SoundEffect::select);
        break;
    }
    }
}

void TitleScreenMode::render(GameContext &ctx) {
    Renderer &r = ctx.ui.getRenderer();

    switch (phase) {
    case Phase::titleCard: {
        // Dark background
        r.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{10, 10, 35});

        // Title text with pulsing effect
        int alpha = std::min(255, titleTimer * 12);
        (void)alpha; // Used conceptually — we just draw when visible

        if (titleTimer > 5) {
            ctx.ui.getSpriteFont().drawText(r, "CIPHERBOUND",
                                            WINDOW_WIDTH / 2 - 11 * 8 * PIXEL_SCALE / 2,
                                            WINDOW_HEIGHT / 2 - 60, PIXEL_SCALE, 1);

            // Subtitle
            r.drawText("A Science Daemon Adventure", WINDOW_WIDTH / 2 - 110, WINDOW_HEIGHT / 2 + 10,
                       TDT4102::Color{160, 160, 200}, 14);
        }

        if (titleTimer > 15 && (titleTimer / 15) % 2 == 0) {
            r.drawText("Press Z or Enter to continue", WINDOW_WIDTH / 2 - 110, WINDOW_HEIGHT - 60,
                       TDT4102::Color{120, 120, 170}, 12);
        }
        break;
    }

    case Phase::saveSlotSelect: {
        Renderer &renderer = ctx.ui.getRenderer();
        SpriteFont &spriteFont = ctx.ui.getSpriteFont();

        renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{20, 20, 60});

        spriteFont.drawText(renderer, "CIPHERBOUND", WINDOW_WIDTH / 2 - 11 * 8 * PIXEL_SCALE / 2,
                            40, PIXEL_SCALE, 1);

        renderer.drawText("Select a save slot", WINDOW_WIDTH / 2 - 70, 90,
                          TDT4102::Color::light_gray, 14);

        int slotBoxW = 600;
        int slotBoxH = 80;
        int startY = 130;
        int spacing = 10;
        int startX = (WINDOW_WIDTH - slotBoxW) / 2;

        int slotCount = static_cast<int>(slotInfos.size());
        for (int i = 0; i < slotCount; ++i) {
            int y = startY + i * (slotBoxH + spacing);
            bool isSelected = i == selected;

            TDT4102::Color bgColor =
                isSelected ? TDT4102::Color{228, 236, 255} : TDT4102::Color{35, 35, 80};
            renderer.drawFilledRect(startX, y, slotBoxW, slotBoxH, bgColor);

            TDT4102::Color borderColor =
                isSelected ? TDT4102::Color{24, 32, 74} : TDT4102::Color{80, 80, 130};
            renderer.drawRect(startX, y, slotBoxW, slotBoxH, TDT4102::Color::transparent,
                              borderColor);

            const TDT4102::Color labelColor =
                isSelected ? TDT4102::Color{12, 18, 48} : TDT4102::Color{220, 225, 245};
            const TDT4102::Color infoColor =
                isSelected ? TDT4102::Color{20, 30, 70} : TDT4102::Color{190, 196, 220};
            const TDT4102::Color emptySlotColor =
                isSelected ? TDT4102::Color{20, 30, 70} : TDT4102::Color{140, 140, 180};

            if (isSelected)
                ctx.ui.drawSelectionArrow(startX + 8, y + slotBoxH / 2 - PIXEL_SCALE * 3,
                                          PIXEL_SCALE);

            int textX = startX + 40;
            int textY = y + 10;

            std::string slotLabel = "Slot " + std::to_string(i + 1);
            renderer.drawText(slotLabel, textX, textY, labelColor, 18);

            const auto &slotInfo = slotInfos[static_cast<std::size_t>(i)];
            if (slotInfo.exists) {
                std::string info = slotInfo.playerName;
                info += "   Party: " + std::to_string(slotInfo.partySize);
                info += "   Badges: " + std::to_string(slotInfo.badgeCount);
                if (!slotInfo.mapId.empty())
                    info += "   Map: " + slotInfo.mapId;
                renderer.drawText(info, textX, textY + 30, infoColor, 14);
            } else {
                renderer.drawText("New Game", textX, textY + 30, emptySlotColor, 14);
            }
        }

        renderer.drawText("Arrow keys to select, Z or Enter to confirm", WINDOW_WIDTH / 2 - 160,
                          WINDOW_HEIGHT - 40, TDT4102::Color{100, 100, 150}, 12);
        break;
    }

    case Phase::nameEntry: {
        Renderer &renderer = ctx.ui.getRenderer();
        SpriteFont &spriteFont = ctx.ui.getSpriteFont();

        renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{20, 20, 60});

        spriteFont.drawText(renderer, "CIPHERBOUND", WINDOW_WIDTH / 2 - 11 * 8 * PIXEL_SCALE / 2,
                            40, PIXEL_SCALE, 1);
        renderer.drawText("Choose a name", WINDOW_WIDTH / 2 - 58, 90, TDT4102::Color::light_gray,
                          14);

        const int nameBoxW = 520;
        const int nameBoxH = 72;
        const int nameBoxX = (WINDOW_WIDTH - nameBoxW) / 2;
        const int nameBoxY = 120;
        renderer.drawFilledRect(nameBoxX, nameBoxY, nameBoxW, nameBoxH,
                                TDT4102::Color{228, 236, 255});
        renderer.drawRect(nameBoxX, nameBoxY, nameBoxW, nameBoxH, TDT4102::Color::transparent,
                          TDT4102::Color{24, 32, 74});
        renderer.drawText("Name", nameBoxX + 18, nameBoxY + 12, TDT4102::Color{20, 30, 70}, 16);

        const std::string shownName = displayPlayerName(pendingName);
        const int nameTextX = nameBoxX + 18;
        const int nameTextY = nameBoxY + 38;
        renderer.drawText(shownName, nameTextX, nameTextY, TDT4102::Color{20, 30, 70}, 28);

        const std::string countText =
            std::to_string(pendingName.size()) + "/" + std::to_string(maxPlayerNameLength);
        renderer.drawText(countText, nameBoxX + nameBoxW - 55, nameBoxY + 18,
                          TDT4102::Color{70, 82, 120}, 14);

        const int cellW = 92;
        const int cellH = 48;
        const int cellGap = 8;
        const int gridW = nameGridColumns * cellW + (nameGridColumns - 1) * cellGap;
        const int gridX = (WINDOW_WIDTH - gridW) / 2;
        const int gridY = 220;

        for (std::size_t i = 0; i < nameKeys.size(); ++i) {
            const int row = static_cast<int>(i) / nameGridColumns;
            const int col = static_cast<int>(i) % nameGridColumns;
            const int x = gridX + col * (cellW + cellGap);
            const int y = gridY + row * (cellH + cellGap);
            const bool isSelected = static_cast<int>(i) == nameKeySelected;

            const TDT4102::Color fill =
                isSelected ? TDT4102::Color{228, 236, 255} : TDT4102::Color{35, 35, 80};
            const TDT4102::Color border =
                isSelected ? TDT4102::Color{24, 32, 74} : TDT4102::Color{80, 80, 130};
            renderer.drawFilledRect(x, y, cellW, cellH, fill);
            renderer.drawRect(x, y, cellW, cellH, TDT4102::Color::transparent, border);

            const std::string label{nameKeys[i]};
            const int labelScale = label.size() > 1 ? 1 : 2;
            const int labelWidth = spriteFont.getTextWidth(label, labelScale);
            const int labelX = x + (cellW - labelWidth) / 2;
            const int labelY = y + (cellH - 8 * labelScale) / 2 - 2;
            spriteFont.drawText(renderer, label, labelX, labelY, labelScale, 1);
        }

        renderer.drawText("Arrows move. Z/Enter selects. X/Esc deletes or goes back.",
                          WINDOW_WIDTH / 2 - 170, WINDOW_HEIGHT - 58, TDT4102::Color{190, 196, 220},
                          13);
        renderer.drawText("Use BACK to return to the save slots and DONE to start.",
                          WINDOW_WIDTH / 2 - 175, WINDOW_HEIGHT - 34, TDT4102::Color{120, 130, 175},
                          12);
        break;
    }
    }
}
