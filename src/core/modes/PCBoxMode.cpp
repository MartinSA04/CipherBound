#include "PCBoxMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../world/World.h"
#include "../../world/Player.h"
#include <algorithm>

void PCBoxMode::onEnter(GameContext & /*ctx*/)
{
    selected = 0;
    viewingParty = true;
    showingMessage = false;
    message.clear();
}

void PCBoxMode::update(GameContext &ctx, InputManager &input)
{
    Player &player = ctx.world.getPlayer();

    // Showing a result message
    if (showingMessage)
    {
        if (ctx.ui.updateTypewriter(input.isConfirmPressed()))
            showingMessage = false;
        return;
    }

    // Left/Right: switch boxes
    Direction dir;
    if (input.getMovementDirection(dir))
    {
        if (dir == Direction::left || dir == Direction::right)
        {
            if (!lrHeld)
            {
                int box = player.getCurrentBox();
                if (dir == Direction::left)
                    box = (box - 1 + Player::NUM_BOXES) % Player::NUM_BOXES;
                else
                    box = (box + 1) % Player::NUM_BOXES;
                player.setCurrentBox(box);
                if (!viewingParty)
                    selected = 0;
                lrHeld = true;
            }
        }
        else
        {
            // Up/Down navigation
            if (viewingParty)
            {
                ctx.ui.navigateVertical(selected, player.partySize());
            }
            else
            {
                int boxCount = player.getBoxCount(player.getCurrentBox());
                if (boxCount > 0)
                    ctx.ui.navigateVertical(selected, boxCount);
            }
        }
    }
    else
    {
        lrHeld = false;
    }

    // Tab between party and box views
    if (input.isMenuPressed())
    {
        viewingParty = !viewingParty;
        selected = 0;
    }

    // Confirm: deposit or withdraw
    if (input.isConfirmPressed())
    {
        if (viewingParty)
        {
            // Deposit: party → box
            if (player.partySize() > 0 && selected < player.partySize())
            {
                if (!player.canDeposit())
                {
                    message = "Can't deposit your last creature!";
                    showingMessage = true;
                    ctx.ui.setDialogueText(message);
                }
                else if (player.getBoxCount(player.getCurrentBox()) >= Player::BOX_SIZE)
                {
                    message = "This box is full!";
                    showingMessage = true;
                    ctx.ui.setDialogueText(message);
                }
                else
                {
                    std::string name = player.getCreature(selected).getNickname();
                    player.depositCreature(selected);
                    message = name + " deposited to Box " +
                              std::to_string(player.getCurrentBox() + 1) + ".";
                    showingMessage = true;
                    ctx.ui.setDialogueText(message);
                    if (selected >= player.partySize())
                        selected = std::max(0, player.partySize() - 1);
                }
            }
        }
        else
        {
            // Withdraw: box → party
            int boxCount = player.getBoxCount(player.getCurrentBox());
            if (boxCount > 0 && selected < boxCount)
            {
                if (!player.canWithdraw())
                {
                    message = "Your party is full!";
                    showingMessage = true;
                    ctx.ui.setDialogueText(message);
                }
                else
                {
                    const auto &box = player.getBox(player.getCurrentBox());
                    std::string name = box[selected].getNickname();
                    player.withdrawCreature(player.getCurrentBox(), selected);
                    message = name + " withdrawn to party.";
                    showingMessage = true;
                    ctx.ui.setDialogueText(message);
                    int newCount = player.getBoxCount(player.getCurrentBox());
                    if (selected >= newCount)
                        selected = std::max(0, newCount - 1);
                }
            }
        }
    }

    // Cancel: exit PC
    if (input.isCancelPressed())
    {
        ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
    }
}

void PCBoxMode::render(GameContext &ctx)
{
    ctx.ui.drawPCBoxScreen(ctx.world.getPlayer(), selected, viewingParty);

    if (showingMessage)
    {
        ctx.ui.drawDialogueBox("PC", message);
    }
}
