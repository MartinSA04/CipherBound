#pragma once
#include "../GameMode.h"

class PartyMode : public GameMode
{
public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

private:
    enum class SubState
    {
        browsing,        // Navigating party list
        actionMenu,      // Showing action sub-menu (Summary, Switch, Cancel)
        selectingSwap,   // Picking second Daemon to swap with
        showingSummary,  // Viewing a Daemon's summary screen
    };

    int selected{0};
    SubState subState{SubState::browsing};
    int actionSelected{0};
    int swapSource{0};      // Index of Daemon to swap from
    int summaryPage{0};     // 0 = info/moves, 1 = stats
};
