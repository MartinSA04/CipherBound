#include "PartyMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../world/World.h"
#include "../../world/Player.h"

void PartyMode::update(GameContext &ctx, InputManager &input)
{
    Player &player = ctx.world.getPlayer();
    int partySize = player.partySize();

    switch (subState)
    {
    case SubState::browsing:
        ctx.ui.navigateVertical(selected, partySize);

        if (input.isConfirmPressed())
        {
            subState = SubState::actionMenu;
            actionSelected = 0;
        }
        if (input.isCancelPressed())
        {
            ctx.pushRequest(ModeRequest::changeState(GameState::menu));
        }
        break;

    case SubState::actionMenu:
        ctx.ui.navigateVertical(actionSelected, 3); // Summary, Switch, Cancel

        if (input.isConfirmPressed())
        {
            switch (actionSelected)
            {
            case 0: // Summary — just go back for now (placeholder)
                subState = SubState::browsing;
                break;
            case 1: // Switch
                if (partySize > 1)
                {
                    swapSource = selected;
                    subState = SubState::selectingSwap;
                }
                else
                {
                    subState = SubState::browsing;
                }
                break;
            case 2: // Cancel
                subState = SubState::browsing;
                break;
            }
        }
        if (input.isCancelPressed())
        {
            subState = SubState::browsing;
        }
        break;

    case SubState::selectingSwap:
        ctx.ui.navigateVertical(selected, partySize);

        if (input.isConfirmPressed())
        {
            if (selected != swapSource)
            {
                player.swapDaemon(swapSource, selected);
            }
            subState = SubState::browsing;
        }
        if (input.isCancelPressed())
        {
            subState = SubState::browsing;
        }
        break;
    }
}

void PartyMode::render(GameContext &ctx)
{
    ctx.ui.drawPartyList(ctx.world.getPlayer(), selected);

    if (subState == SubState::actionMenu)
    {
        ctx.ui.drawChoiceBox({"Summary", "Switch", "Cancel"}, actionSelected);
    }
    else if (subState == SubState::selectingSwap)
    {
        // Draw indicator of which Daemon is being swapped
        ctx.ui.drawDialogueBox("", "Choose a Daemon to swap with.");
    }
}
