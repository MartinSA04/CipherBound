#include "MessageState.h"
#include "../../../audio/SoundManager.h"
#include "../../../state/Player.h"
#include "../../../state/World.h"
#include "../../../ui/GameUI.h"
#include "../../../ui/InputManager.h"
#include "../BagMode.h"

void MessageState::update(BagMode &bag, GameContext &ctx, InputManager &input) {
    if (ctx.ui.updateTypewriter(input.isConfirmPressed())) {
        ctx.playSound(SoundEffect::select);
        Player &player = ctx.world.getPlayer();
        if (player.getInventory().empty()) {
            ctx.pushRequest(ModeRequest::changeState(GameState::menu));
        } else {
            bag.switchSubState(bag.returnAfterMessage);
        }
    }
}

void MessageState::render(BagMode &bag, GameContext &ctx) {
    ctx.ui.drawBagScreen(ctx.world.getPlayer(), ctx.pokedex, bag.selected);
    ctx.ui.drawDialogueBox("", bag.message);
}
