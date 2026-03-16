#include "BrowsingState.h"
#include "../../../data/Pokedex.h"
#include "../../../state/Player.h"
#include "../../../state/World.h"
#include "../../../ui/GameUI.h"
#include "../../../ui/InputManager.h"
#include "../BagMode.h"

void BrowsingState::update(BagMode &bag, GameContext &ctx,
                           InputManager &input) {
    Player &player = ctx.world.getPlayer();
    int itemCount = static_cast<int>(player.getInventory().size());

    if (itemCount == 0) {
        if (input.isCancelPressed())
            ctx.pushRequest(ModeRequest::changeState(GameState::menu));
        return;
    }

    ctx.ui.navigateVertical(bag.selected, itemCount);

    if (input.isConfirmPressed()) {
        const auto &inv = player.getInventory();
        if (bag.selected >= 0 &&
            static_cast<std::size_t>(bag.selected) < inv.size()) {
            const ItemData &item = ctx.pokedex.getItem(
                inv[static_cast<std::size_t>(bag.selected)].itemId);
            if (item.category == ItemCategory::healing) {
                bag.useItemId =
                    inv[static_cast<std::size_t>(bag.selected)].itemId;
                bag.partySelected = 0;
                bag.switchSubState(BagMode::SubStateType::choosingTarget);
            } else {
                bag.showMessage("Can't use that here!", ctx);
            }
        }
    }

    if (input.isCancelPressed()) {
        ctx.pushRequest(ModeRequest::changeState(GameState::menu));
    }
}

void BrowsingState::render(BagMode &bag, GameContext &ctx) {
    ctx.ui.drawBagScreen(ctx.world.getPlayer(), ctx.pokedex, bag.selected);
}
