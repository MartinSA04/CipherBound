#include "BagMode.h"
#include "../../ui/GameUI.h"
#include "bag/BrowsingState.h"
#include "bag/MessageState.h"
#include "bag/TargetState.h"

BagMode::BagMode() : currentSubState(std::make_unique<BrowsingState>()) {}

void BagMode::update(GameContext &ctx, InputManager &input) {
    if (currentSubState)
        currentSubState->update(*this, ctx, input);
}

void BagMode::render(GameContext &ctx) {
    if (currentSubState)
        currentSubState->render(*this, ctx);
}

void BagMode::switchSubState(SubStateType type) { currentSubState = createSubState(type); }

void BagMode::showMessage(const std::string &msg, GameContext &ctx) {
    message = msg;
    ctx.ui.setDialogueText(msg);
    returnAfterMessage = SubStateType::browsing;
    switchSubState(SubStateType::showingMessage);
}

std::unique_ptr<BagSubState> BagMode::createSubState(SubStateType type) {
    switch (type) {
    case SubStateType::browsing:
        return std::make_unique<BrowsingState>();
    case SubStateType::choosingTarget:
        return std::make_unique<TargetState>();
    case SubStateType::showingMessage:
        return std::make_unique<MessageState>();
    }
    return std::make_unique<BrowsingState>();
}
