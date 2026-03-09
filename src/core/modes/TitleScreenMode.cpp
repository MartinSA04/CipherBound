#include "TitleScreenMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../ui/Renderer.h"
#include "../../ui/SpriteFont.h"
#include "../../save/SaveManager.h"
#include "../../world/World.h"
#include "../../world/Player.h"
#include "../../data/Pokedex.h"
#include "../../audio/MusicManager.h"
#include <algorithm>

void TitleScreenMode::onEnter(GameContext &ctx)
{
    slotInfos = ctx.saveManager.getSlotInfos();
    selected = 0;
    phase = Phase::titleCard;
    titleTimer = 0;
}

void TitleScreenMode::update(GameContext &ctx, InputManager &input)
{
    switch (phase)
    {
    case Phase::titleCard:
    {
        titleTimer++;
        // Wait at least 30 frames before allowing skip, then any key proceeds
        if (titleTimer > 30 && (input.isConfirmPressed() || input.isCancelPressed()))
        {
            phase = Phase::saveSlotSelect;
        }
        break;
    }

    case Phase::saveSlotSelect:
    {
        ctx.ui.navigateVertical(selected, ctx.saveManager.getSlotCount());

        if (input.isConfirmPressed())
        {
            ctx.currentSaveSlot = selected;
            const auto &info = slotInfos[static_cast<size_t>(ctx.currentSaveSlot)];

            if (info.exists)
            {
                ctx.saveManager.loadGame(
                    ctx.saveManager.getSavePath(ctx.currentSaveSlot),
                    ctx.world.getPlayer(), ctx.world, ctx.pokedex);
                ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
            }
            else
            {
                // New game — give starter items
                constexpr int potionId = 1;
                constexpr int daemonBallId = 5;
                ctx.world.getPlayer().addItem(potionId, 5);
                ctx.world.getPlayer().addItem(daemonBallId, 10);
                ctx.pushRequest(ModeRequest::dialogue("", {"What a good night sleep!", "Its finally my birthday and time to start my adventure!", "I should go see Bart Iver in his lab"}));
            }

            MusicTrack mapTrack = MusicManager::trackForMap(ctx.world.getCurrentMapId());
            ctx.music.play(mapTrack, ctx.ui.getRenderer().getWindow());
        }
        break;
    }
    }
}

void TitleScreenMode::render(GameContext &ctx)
{
    Renderer &r = ctx.ui.getRenderer();

    switch (phase)
    {
    case Phase::titleCard:
    {
        // Dark background
        r.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{10, 10, 35});

        // Title text with pulsing effect
        int alpha = std::min(255, titleTimer * 6);
        (void)alpha; // Used conceptually — we just draw when visible

        if (titleTimer > 10)
        {
            ctx.ui.getSpriteFont().drawText(r, "CIPHERBOUND",
                                            WINDOW_WIDTH / 2 - 11 * 8 * PIXEL_SCALE / 2,
                                            WINDOW_HEIGHT / 2 - 60, PIXEL_SCALE, 1);

            // Subtitle
            r.drawText("A Science Daemon Adventure",
                       WINDOW_WIDTH / 2 - 110, WINDOW_HEIGHT / 2 + 10,
                       TDT4102::Color{160, 160, 200}, 14);
        }

        // "Press any key" blink
        if (titleTimer > 30 && (titleTimer / 30) % 2 == 0)
        {
            r.drawText("Press Z or Enter to continue",
                       WINDOW_WIDTH / 2 - 110, WINDOW_HEIGHT - 60,
                       TDT4102::Color{120, 120, 170}, 12);
        }
        break;
    }
    case Phase::saveSlotSelect:
    {
        Renderer &renderer = ctx.ui.getRenderer();
        SpriteFont &spriteFont = ctx.ui.getSpriteFont();

        renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{20, 20, 60});

        // Title
        spriteFont.drawText(renderer, "CIPHERBOUND",
                            WINDOW_WIDTH / 2 - 8 * 16 * PIXEL_SCALE / 2, 40, PIXEL_SCALE, 1);

        // Subtitle
        renderer.drawText("Select a save slot", WINDOW_WIDTH / 2 - 70, 90,
                          TDT4102::Color::light_gray, 14);

        // Draw save slots
        int slotBoxW = 600;
        int slotBoxH = 80;
        int startY = 130;
        int spacing = 10;
        int startX = (WINDOW_WIDTH - slotBoxW) / 2;

        for (size_t i = 0; i < slotInfos.size(); ++i)
        {
            int y = startY + static_cast<int>(i) * (slotBoxH + spacing);
            bool isSelected = (static_cast<int>(i) == selected);

            TDT4102::Color bgColor = isSelected
                                         ? TDT4102::Color{60, 60, 120}
                                         : TDT4102::Color{35, 35, 80};
            renderer.drawFilledRect(startX, y, slotBoxW, slotBoxH, bgColor);

            TDT4102::Color borderColor = isSelected
                                             ? TDT4102::Color{200, 200, 255}
                                             : TDT4102::Color{80, 80, 130};
            renderer.drawRect(startX, y, slotBoxW, slotBoxH, borderColor);

            if (isSelected)
                ctx.ui.drawSelectionArrow(startX + 8, y + slotBoxH / 2 - PIXEL_SCALE * 3, PIXEL_SCALE);

            int textX = startX + 40;
            int textY = y + 10;

            std::string slotLabel = "Slot " + std::to_string(i + 1);
            renderer.drawText(slotLabel, textX, textY, TDT4102::Color::white, 18);

            if (slotInfos[i].exists)
            {
                std::string info = slotInfos[i].playerName;
                info += "   Party: " + std::to_string(slotInfos[i].partySize);
                info += "   Badges: " + std::to_string(slotInfos[i].badgeCount);
                if (!slotInfos[i].mapId.empty())
                    info += "   Map: " + slotInfos[i].mapId;
                renderer.drawText(info, textX, textY + 30, TDT4102::Color::light_gray, 14);
            }
            else
            {
                renderer.drawText("New Game", textX, textY + 30, TDT4102::Color{140, 140, 180}, 14);
            }
        }

        // Controls hint
        renderer.drawText("Arrow keys to select, Z or Enter to confirm",
                          WINDOW_WIDTH / 2 - 160, WINDOW_HEIGHT - 40,
                          TDT4102::Color{100, 100, 150}, 12);
        break;
    }
    }
}
