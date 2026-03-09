#pragma once
#include "../GameMode.h"
#include "bag/BagSubState.h"
#include <memory>
#include <string>

class BagMode : public GameMode
{
public:
    enum class SubStateType
    {
        browsing,
        choosingTarget,
        showingMessage,
    };

    BagMode();

    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

    // Sub-state transitions
    void switchSubState(SubStateType type);
    void showMessage(const std::string &msg, GameContext &ctx);

    // Shared state accessible by sub-states
    int selected{0};
    int partySelected{0};
    int useItemId{0};
    std::string message;

    // After a message, which sub-state to return to
    SubStateType returnAfterMessage{SubStateType::browsing};

private:
    std::unique_ptr<BagSubState> createSubState(SubStateType type);
    std::unique_ptr<BagSubState> currentSubState;
};
