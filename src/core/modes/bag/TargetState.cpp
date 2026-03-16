#include "TargetState.h"
#include "../../../audio/SoundManager.h"
#include "../../../data/Pokedex.h"
#include "../../../state/Daemon.h"
#include "../../../state/Player.h"
#include "../../../state/World.h"
#include "../../../ui/GameUI.h"
#include "../../../ui/InputManager.h"
#include "../BagMode.h"

void TargetState::update(BagMode &bag, GameContext &ctx, InputManager &input) {
    Player &player = ctx.world.getPlayer();
    ctx.ui.navigateVertical(bag.partySelected, player.partySize());

    if (input.isConfirmPressed()) {
        Daemon &target = player.getDaemon(bag.partySelected);
        const ItemData &item = ctx.pokedex.getItem(bag.useItemId);

        if (target.isFainted()) {
            bag.showMessage(target.getNickname() + " has no energy!", ctx);
        } else if (target.getCurrentHP() >= target.getMaxHP()) {
            bag.showMessage(target.getNickname() + "'s HP is already full!", ctx);
        } else {
            int before = target.getCurrentHP();
            if (item.effectValue >= 9999)
                target.fullHeal();
            else
                target.heal(item.effectValue);
            int healed = target.getCurrentHP() - before;

            player.removeItem(bag.useItemId, 1);
            int newSize = static_cast<int>(player.getInventory().size());
            if (newSize == 0)
                bag.selected = 0;
            else if (bag.selected >= newSize)
                bag.selected = newSize - 1;

            bag.showMessage(target.getNickname() + " recovered " + std::to_string(healed) + " HP!", ctx);
            ctx.playSound(SoundEffect::recovery);
            // After using item, return to browsing (via message)
            bag.returnAfterMessage = BagMode::SubStateType::browsing;
        }
    }

    if (input.isCancelPressed()) {
        bag.switchSubState(BagMode::SubStateType::browsing);
    }
}

void TargetState::render(BagMode &bag, GameContext &ctx) {
    ctx.ui.drawBagScreen(ctx.world.getPlayer(), ctx.pokedex, bag.selected);
    ctx.ui.drawPartyList(ctx.world.getPlayer(), bag.partySelected);
}
