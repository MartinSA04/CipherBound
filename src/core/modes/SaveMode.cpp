#include "SaveMode.h"
#include "../../audio/SoundManager.h"
#include "../../save/SaveManager.h"
#include "../../state/World.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"

void SaveMode::onEnter(GameContext & /*ctx*/) {
    saveComplete = false;
    saveSuccess = false;
}

void SaveMode::update(GameContext &ctx, InputManager &input) {
    if (!saveComplete) {
        if (ctx.currentSaveSlot < 0) {
            saveSuccess = false;
        } else {
            saveSuccess = ctx.saveManager.saveGame(ctx.saveManager.getSavePath(ctx.currentSaveSlot),
                                                   ctx.world.getPlayer(), ctx.world);
        }
        saveComplete = true;
        if (saveSuccess) {
            ctx.playSound(SoundEffect::save);
            ctx.ui.setDialogueText("Game saved!");
        } else
            ctx.ui.setDialogueText("Save failed...");
    }

    if (ctx.ui.updateTypewriter(input.isConfirmPressed())) {
        ctx.playSound(SoundEffect::select);
        ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
    }
}

void SaveMode::render(GameContext &ctx) {
    renderOverworld(ctx);
    ctx.ui.drawDialogueBox("", saveSuccess ? "Game saved!" : "Save failed...");
}
