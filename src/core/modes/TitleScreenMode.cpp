#include "TitleScreenMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../save/SaveManager.h"
#include "../../world/World.h"
#include "../../world/Player.h"
#include "../../data/Pokedex.h"
#include "../../audio/MusicManager.h"

void TitleScreenMode::onEnter(GameContext &ctx)
{
    slotInfos = ctx.saveManager.getSlotInfos();
    selected = 0;
}

void TitleScreenMode::update(GameContext &ctx, InputManager &input)
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
        }
        else
        {
            // New game — give starter items
            constexpr int potionId = 1;
            constexpr int creatureBallId = 5;
            ctx.world.getPlayer().addItem(potionId, 5);
            ctx.world.getPlayer().addItem(creatureBallId, 10);
        }

        ctx.pushRequest(ModeRequest::changeState(GameState::overworld));

        // Play music for the current map
        MusicTrack mapTrack = MusicManager::trackForMap(ctx.world.getCurrentMapId());
        ctx.music.play(mapTrack, ctx.ui.getRenderer().getWindow());
    }
}

void TitleScreenMode::render(GameContext &ctx)
{
    ctx.ui.drawTitleScreen(slotInfos, selected);
}
