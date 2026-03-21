#include "PartyMode.h"
#include "../../state/World.h"
#include "../../state/player/Player.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"

void PartyMode::update(GameContext &ctx, InputManager &input) {
    Player &player = ctx.world.getPlayer();
    int partySize = player.partySize();

    switch (subState) {
    case SubState::browsing:
        ctx.ui.navigateVertical(selected, partySize);

        if (input.isConfirmPressed() && partySize > 0) {
            ctx.playSound(SoundEffect::select);
            subState = SubState::actionMenu;
            actionSelected = 0;
        }
        if (input.isCancelPressed()) {
            ctx.pushRequest(ModeRequest::changeState(GameState::menu));
        }
        break;

    case SubState::actionMenu:
        ctx.ui.navigateVertical(actionSelected, 3); // Summary, Switch, Cancel

        if (input.isConfirmPressed()) {
            ctx.playSound(SoundEffect::select);
            switch (actionSelected) {
            case 0: // Summary
                summaryPage = 0;
                summaryMoveSelected = 0;
                subState = SubState::showingSummary;
                break;
            case 1: // Switch
                if (partySize > 1) {
                    swapSource = selected;
                    subState = SubState::selectingSwap;
                } else {
                    subState = SubState::browsing;
                }
                break;
            case 2: // Cancel
                subState = SubState::browsing;
                break;
            }
        }
        if (input.isCancelPressed()) {
            subState = SubState::browsing;
        }
        break;

    case SubState::selectingSwap:
        ctx.ui.navigateVertical(selected, partySize);

        if (input.isConfirmPressed()) {
            ctx.playSound(SoundEffect::select);
            if (selected != swapSource) {
                player.swapDaemon(swapSource, selected);
            }
            subState = SubState::browsing;
        }
        if (input.isCancelPressed()) {
            subState = SubState::browsing;
        }
        break;

    case SubState::showingSummary: {
        Direction dir;
        const bool dirHeld = input.getMovementDirection(dir);
        if (dirHeld && (dir == Direction::left || dir == Direction::right))
            ctx.ui.navigateHorizontal(summaryPage, 3);
        else if (summaryPage == 2)
            ctx.ui.navigateVertical(summaryMoveSelected, 4);
        else
            ctx.ui.navigateVertical(selected, partySize);
        if (input.isCancelPressed()) {
            subState = SubState::browsing;
        }
        break;
    }
    }
}

void PartyMode::render(GameContext &ctx) {
    if (subState == SubState::showingSummary) {
        const Daemon &daemon = ctx.world.getPlayer().getDaemon(selected);
        ctx.ui.drawSummaryScreen(daemon, ctx.pokedex, summaryPage, summaryMoveSelected);
        return;
    }

    ctx.ui.drawPartyList(ctx.world.getPlayer(), selected);

    if (subState == SubState::actionMenu) {
        ctx.ui.drawChoiceBox({"Summary", "Switch", "Cancel"}, actionSelected);
    } else if (subState == SubState::selectingSwap) {
        ctx.ui.drawDialogueBox("", "Choose a Daemon to swap with.");
    }
}
