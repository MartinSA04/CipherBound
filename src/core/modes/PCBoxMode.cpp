#include "PCBoxMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../ui/Renderer.h"
#include "../../ui/SpriteFont.h"
#include "../../state/World.h"
#include "../../state/Player.h"
#include "../../state/Daemon.h"
#include "../../audio/SoundManager.h"
#include <algorithm>

void PCBoxMode::onEnter(GameContext &ctx)
{
    selected = 0;
    viewingParty = true;
    showingMessage = false;
    message.clear();
    ctx.playSound(SoundEffect::pcLogin);
}

void PCBoxMode::update(GameContext &ctx, InputManager &input)
{
    Player &player = ctx.world.getPlayer();

    // Showing a result message
    if (showingMessage)
    {
        if (ctx.ui.updateTypewriter(input.isConfirmPressed()))
        {
            ctx.playSound(SoundEffect::select);
            showingMessage = false;
        }
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
                    message = "Can't deposit your last Daemon!";
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
                    std::string name = player.getDaemon(selected).getNickname();
                    player.depositDaemon(selected);
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
                    player.withdrawDaemon(player.getCurrentBox(), selected);
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
        ctx.playSound(SoundEffect::pcLogoff);
        ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
    }
}

void PCBoxMode::render(GameContext &ctx)
{
    Renderer &renderer = ctx.ui.getRenderer();
    SpriteFont &spriteFont = ctx.ui.getSpriteFont();
    const Player &player = ctx.world.getPlayer();

    int scale = PIXEL_SCALE;
    int halfW = WINDOW_WIDTH / 2;

    // Background
    renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color{50, 60, 90});

    // Title: "Box N" with left/right arrows
    std::string boxTitle = "< Box " + std::to_string(player.getCurrentBox() + 1) + " >";
    int titleX = halfW - static_cast<int>(boxTitle.size()) * 4 * scale;
    spriteFont.drawText(renderer, boxTitle, titleX, 8, scale);

    // --- Left panel: Party ---
    int panelY = 12 * scale;
    spriteFont.drawText(renderer, "PARTY", 28, panelY, scale);
    panelY += 12 * scale;

    int slotH = 12 * scale;
    int slotGap = 4;
    const auto &party = player.getParty();

    for (int i = 0; i < 6; ++i)
    {
        int sy = panelY + i * (slotH + slotGap);
        bool hasDaemon = i < static_cast<int>(party.size());

        TDT4102::Color bg = TDT4102::Color{70, 80, 110};
        if (viewingParty && i == selected)
            bg = TDT4102::Color{140, 160, 220};

        renderer.drawFilledRect(10, sy, halfW - 20, slotH, bg);
        renderer.drawRect(10, sy, halfW - 20, slotH,
                          TDT4102::Color::transparent, TDT4102::Color{100, 110, 140});

        if (hasDaemon)
        {
            const Daemon &c = party[i];
            if (viewingParty && i == selected)
                ctx.ui.drawSelectionArrow(16, sy + 2 * scale, scale);

            spriteFont.drawText(renderer, c.getNickname(), 16 + 6 * scale, sy + 2, scale);
            spriteFont.drawText(renderer, "Lv" + std::to_string(c.getLevel()),
                                halfW - 80 - 12 * scale, sy + 2, scale);
        }
        else
        {
            spriteFont.drawText(renderer, "---", 16 + 6 * scale, sy + 2, scale);
        }
    }

    // --- Right panel: Box contents ---
    int boxPanelX = halfW + 10;
    spriteFont.drawText(renderer, "BOX", boxPanelX + 8, 12 * scale, scale);
    int boxPanelY = 16 * scale + 8 * scale;

    const auto &box = player.getBox(player.getCurrentBox());
    int boxCount = static_cast<int>(box.size());

    int maxVisible = 10;
    int scrollOffset = 0;
    if (!viewingParty && selected >= maxVisible)
        scrollOffset = selected - maxVisible + 1;

    for (int i = 0; i < maxVisible && (i + scrollOffset) < Player::BOX_SIZE; ++i)
    {
        int idx = i + scrollOffset;
        int sy = boxPanelY + i * (slotH + slotGap);
        bool hasDaemon = idx < boxCount;

        TDT4102::Color bg = TDT4102::Color{70, 80, 110};
        if (!viewingParty && idx == selected)
            bg = TDT4102::Color{140, 160, 220};

        renderer.drawFilledRect(boxPanelX, sy, halfW - 20, slotH, bg);
        renderer.drawRect(boxPanelX, sy, halfW - 20, slotH,
                          TDT4102::Color::transparent, TDT4102::Color{100, 110, 140});

        if (hasDaemon)
        {
            const Daemon &c = box[idx];
            if (!viewingParty && idx == selected)
                ctx.ui.drawSelectionArrow(boxPanelX + 6, sy + 2 * scale, scale);

            spriteFont.drawText(renderer, c.getNickname(), boxPanelX + 6 + 6 * scale, sy + 2, scale);
            spriteFont.drawText(renderer, "Lv" + std::to_string(c.getLevel()),
                                WINDOW_WIDTH - 80 - 12 * scale, sy + 2, scale);
        }
        else
        {
            spriteFont.drawText(renderer, "---", boxPanelX + 6 + 6 * scale, sy + 2, scale);
        }
    }

    // Status bar at bottom
    int statusY = WINDOW_HEIGHT - 10 * scale;
    if (viewingParty)
        spriteFont.drawText(renderer, "A:Deposit  LR:Switch Box  B:Exit", 20, statusY, scale - 1);
    else
        spriteFont.drawText(renderer, "A:Withdraw  LR:Switch Box  B:Exit", 20, statusY, scale - 1);

    if (showingMessage)
    {
        ctx.ui.drawDialogueBox("PC", message);
    }
}
