#include "OverworldMode.h"
#include "../../state/player/Player.h"
#include "../../story/StoryManager.h"
#include "../../ui/InputManager.h"

void OverworldMode::onEnter(GameContext &ctx) {
    StoryAction mapAction = ctx.story.checkMapEnter(ctx.world);
    if (!mapAction.is<StoryNoAction>())
        ctx.pushRequest(ModeRequest::storyAction(std::move(mapAction)));
}

void OverworldMode::update(GameContext &ctx, InputManager &input) {
    Player &player = ctx.world.getPlayer();
    player.updateAnimation();

    if (wildBattleStarts(ctx))
        return;

    if (warpBlockStarts(ctx))
        return;

    if (trainerBattleStarts(ctx))
        return;

    handlePlayerMove(ctx, input);

    if (input.isMenuPressed())
        ctx.pushRequest(ModeRequest::changeState(GameState::menu));

    if (input.isConfirmPressed())
        handlePlayerInteraction(ctx);
}

void OverworldMode::render(GameContext &ctx) { renderOverworld(ctx); }
