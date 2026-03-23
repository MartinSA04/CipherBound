#include "ShopMode.h"
#include "../../audio/SoundEffects.h"
#include "../../game_data/Pokedex.h"
#include "../../state/World.h"
#include "../../state/player/Player.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"

ShopMode::ShopMode(std::string title, std::string shopkeeper, std::vector<int> items)
    : shopTitle(std::move(title)), shopkeeperName(std::move(shopkeeper)),
      itemIds(std::move(items)) {}

int ShopMode::maxPurchaseQuantity(const Player &player, const ItemData &item) const {
    constexpr int maxShopQuantity = 99;
    if (item.value <= 0)
        return maxShopQuantity;

    const int affordable = player.getMoney() / item.value;
    if (affordable <= 0)
        return 1;
    return std::min(maxShopQuantity, affordable);
}

int ShopMode::currentPurchaseTotal(const Pokedex &pokedex) const {
    return selectedItem(pokedex).value * purchaseQuantity;
}

const ItemData &ShopMode::selectedItem(const Pokedex &pokedex) const {
    return pokedex.getItem(itemIds[static_cast<std::size_t>(selected)]);
}

void ShopMode::onEnter(GameContext &ctx) {
    message.clear();
    messageActive = false;
    phase = Phase::browsing;
    purchaseQuantity = 1;
    confirmSelected = 0;
    if (selected < 0 || selected >= static_cast<int>(itemIds.size()))
        selected = 0;
    ctx.ui.setDialogueText("");
}

void ShopMode::showMessage(GameContext &ctx, std::string text) {
    message = std::move(text);
    messageActive = true;
    ctx.ui.setDialogueText(message);
}

void ShopMode::update(GameContext &ctx, InputManager &input) {
    if (messageActive) {
        const bool dismissPressed = input.isConfirmPressed() || input.isCancelPressed();
        if (ctx.ui.updateTypewriter(dismissPressed)) {
            ctx.playSound(SoundEffect::select);
            messageActive = false;
        }
        return;
    }

    if (itemIds.empty()) {
        if (input.isConfirmPressed() || input.isCancelPressed())
            ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
        return;
    }

    Player &player = ctx.world.getPlayer();
    const ItemData &item = selectedItem(ctx.pokedex);

    switch (phase) {
    case Phase::browsing:
        ctx.ui.navigateVertical(selected, static_cast<int>(itemIds.size()));

        if (input.isCancelPressed()) {
            ctx.playSound(SoundEffect::select);
            ctx.pushRequest(ModeRequest::changeState(GameState::overworld));
            return;
        }

        if (!input.isConfirmPressed())
            return;

        purchaseQuantity = 1;
        confirmSelected = 0;
        phase = Phase::choosingQuantity;
        ctx.playSound(SoundEffect::select);
        return;

    case Phase::choosingQuantity: {
        int quantityIndex = purchaseQuantity - 1;
        ctx.ui.navigateLinear(quantityIndex, maxPurchaseQuantity(player, item));
        purchaseQuantity = quantityIndex + 1;

        if (input.isCancelPressed()) {
            phase = Phase::browsing;
            ctx.playSound(SoundEffect::select);
            return;
        }

        if (!input.isConfirmPressed())
            return;

        confirmSelected = 0;
        phase = Phase::confirmingPurchase;
        ctx.playSound(SoundEffect::select);
        return;
    }

    case Phase::confirmingPurchase:
        ctx.ui.navigateVertical(confirmSelected, 2);

        if (input.isCancelPressed()) {
            phase = Phase::choosingQuantity;
            ctx.playSound(SoundEffect::select);
            return;
        }

        if (!input.isConfirmPressed())
            return;

        if (confirmSelected != 0) {
            phase = Phase::choosingQuantity;
            ctx.playSound(SoundEffect::select);
            return;
        }

        if (!player.spendMoney(currentPurchaseTotal(ctx.pokedex))) {
            ctx.playSound(SoundEffect::wallHit);
            phase = Phase::browsing;
            showMessage(ctx, "You don't have enough money.");
            return;
        }

        player.addItem(item.id, purchaseQuantity);
        ctx.playSound(SoundEffect::confirm);
        phase = Phase::browsing;
        showMessage(ctx, "Bought " + item.name + " x" + std::to_string(purchaseQuantity) + "!");
        return;
    }
}

void ShopMode::render(GameContext &ctx) {
    std::string footerText = "Z: Qty  X: Leave";
    if (phase == Phase::choosingQuantity)
        footerText = "Arrows: Qty  Z: Next  X: Back";
    else if (phase == Phase::confirmingPurchase)
        footerText = "Z: Confirm  X: Back";

    ctx.ui.drawShopScreen(ctx.world.getPlayer(), ctx.pokedex, itemIds, selected, shopTitle,
                          footerText);

    if (messageActive) {
        ctx.ui.drawDialogueBox(shopkeeperName, message);
        return;
    }

    if (itemIds.empty())
        return;

    if (phase == Phase::choosingQuantity) {
        const std::string prompt = "Select quantity";
        ctx.ui.setDialogueText(prompt);
        ctx.ui.revealAllText();
        ctx.ui.drawDialogueBox(shopkeeperName, prompt);
        ctx.ui.drawShopQuantityBox(purchaseQuantity);
        return;
    }

    if (phase == Phase::confirmingPurchase) {
        const ItemData &item = selectedItem(ctx.pokedex);
        const std::string prompt = "Buy " + item.name + " x" + std::to_string(purchaseQuantity) +
                                   " for $" + std::to_string(currentPurchaseTotal(ctx.pokedex)) +
                                   "?";
        ctx.ui.setDialogueText(prompt);
        ctx.ui.revealAllText();
        ctx.ui.drawDialogueBox(shopkeeperName, prompt);
        ctx.ui.drawChoiceBox({"Yes", "No"}, confirmSelected);
    }
}
