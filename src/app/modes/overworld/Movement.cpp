#include "../OverworldMode.h"
#include "../../../state/World.h"
#include "../../../state/player/Player.h"
#include "../../../ui/InputManager.h"
#include "../../../story/StoryManager.h"

void OverworldMode::handlePlayerMove(GameContext &ctx, InputManager &input) {
    Player &player = ctx.world.getPlayer();

    Direction dir;
    if (!input.getMovementDirection(dir)) {
        wallHitPlayed = false;
        return;
    }

    if (dir != player.getFacing() && player.canTurn()) {
        player.setFacing(dir);
        if (!player.wasRecentlyMoving())
            player.startTurnCooldown();
        return;
    }

    if (!player.canStep())
        return;

    Map &map = ctx.world.getMap(ctx.world.getCurrentMapId());
    if (!player.canMove(dir, map)) {
        if (!wallHitPlayed || dir != wallHitDir) {
            ctx.playSound(SoundEffect::wallHit);
            wallHitPlayed = true;
            wallHitDir = dir;
        }
        return;
    }

    wallHitPlayed = false;

    Position target = player.getPosition();
    target.moveDirection(dir);
    if (ctx.world.findNPCAt(ctx.world.getCurrentMapId(), target))
        return;

    map.setOccupied(player.getPosition(), false);
    player.move(dir);
    map.setOccupied(player.getPosition(), true);
    justStepped = true;

    handlePlayerWarpAttempt(ctx);
}

void OverworldMode::handlePlayerWarpAttempt(GameContext &ctx) {
    Player &player = ctx.world.getPlayer();
    Map &map = ctx.world.getMap(ctx.world.getCurrentMapId());

    const WarpPoint *warp = map.getWarp(player.getPosition());
    if (!warp)
        return;

    StoryAction block = ctx.story.checkWarp(*warp, player);
    if (const auto *blocked = block.tryGet<StoryBlockWarpAction>()) {
        pendingWarpBlock = true;
        pendingWarpBlockAction = *blocked;
        return;
    }

    ctx.pushRequest(ModeRequest::transition(warp->targetMapId, warp->targetPosition));
}
