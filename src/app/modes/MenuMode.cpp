#include "MenuMode.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"

void MenuMode::update(GameContext &ctx, InputManager &input) {
    ctx.ui.navigateVertical(selected, 6);

    if (input.isCancelPressed()) {
        ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
    }

    if (input.isConfirmPressed()) {
        ctx.playSound(SoundEffect::select);
        switch (selected) {
        case 0:
            ctx.pushRequest(ModeRequest::changeState(GameState::daemondex));
            break;
        case 1:
            ctx.pushRequest(ModeRequest::changeState(GameState::playerStats));
            break;
        case 2:
            ctx.pushRequest(ModeRequest::changeState(GameState::party));
            break;
        case 3:
            ctx.pushRequest(ModeRequest::changeState(GameState::bag));
            break;
        case 4:
            ctx.pushRequest(ModeRequest::changeState(GameState::saving));
            break;
        case 5:
            ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
            break;
        }
    }
}
