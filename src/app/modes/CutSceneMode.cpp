#include "CutSceneMode.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"
#include "../../cutscene/CutsceneRunner.h"

void CutSceneMode::update(GameContext &ctx, InputManager &input) {
    bool running =
        ctx.cutsceneRunner.update(ctx.world, ctx.ui, input.isConfirmPressed(), ctx.sound);
    if (!running) {
        if (ctx.flow.cutsceneEndRequest.has_value()) {
            ctx.pushRequest(std::move(*ctx.flow.cutsceneEndRequest));
            ctx.flow.cutsceneEndRequest.reset();
        } else {
            ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
        }
    }
}

void CutSceneMode::render(GameContext &ctx) {
    // Draw the overworld as the backdrop
    renderOverworld(ctx);

    // If the cutscene is showing dialogue, draw the dialogue box on top
    if (ctx.cutsceneRunner.isShowingDialogue() && ctx.ui.isDialogueActive()) {
        ctx.ui.drawDialogueBox(ctx.ui.getDialogueSpeaker(), ctx.ui.getCurrentDialogueLine());
    }
}
