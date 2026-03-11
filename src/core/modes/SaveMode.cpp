#include "SaveMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../save/SaveManager.h"
#include "../../world/World.h"
#include "../../world/Player.h"
#include "../../audio/SoundManager.h"

void SaveMode::onEnter(GameContext & /*ctx*/)
{
    saveComplete = false;
    saveSuccess = false;
}

void SaveMode::update(GameContext &ctx, InputManager &input)
{
    if (!saveComplete)
    {
        saveSuccess = ctx.saveManager.saveGame(
            ctx.saveManager.getSavePath(ctx.currentSaveSlot),
            ctx.world.getPlayer(), ctx.world);
        saveComplete = true;
        if (saveSuccess)
        {
            ctx.playSound(SoundEffect::save);
            ctx.ui.setDialogueText("Game saved!");
        }
        else
            ctx.ui.setDialogueText("Save failed...");
    }

    if (ctx.ui.updateTypewriter(input.isConfirmPressed()))
    {
        ctx.playSound(SoundEffect::select);
        ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
    }
}

void SaveMode::render(GameContext &ctx)
{
    renderOverworld(ctx);
    ctx.ui.drawDialogueBox("", saveSuccess ? "Game saved!" : "Save failed...");
}
