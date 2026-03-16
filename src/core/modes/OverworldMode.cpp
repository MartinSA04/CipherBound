#include "OverworldMode.h"
#include "../../audio/SoundManager.h"
#include "../../state/NPC.h"
#include "../../state/Player.h"
#include "../../state/World.h"
#include "../../ui/InputManager.h"
#include "../StoryManager.h"

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

// ── Helpers
// ────────────────────────────────────────────────────────────────────

bool OverworldMode::wildBattleStarts(GameContext &ctx) {
    Player &player = ctx.world.getPlayer();
    if (player.isMoving())
        return false;

    // Only roll once per step (not every idle frame)
    if (!justStepped)
        return false;
    justStepped = false;

    Map &map = ctx.world.getMap(ctx.world.getCurrentMapId());
    const Tile &tile = map.getTile(player.getPosition());
    if (tile.type != TileType::tallGrass || !map.hasWildEncounters())
        return false;

    if (!ctx.world.rollWildEncounter(player.getPosition()))
        return false;

    int species = ctx.world.getWildSpecies(player.getPosition());
    int level = ctx.world.getWildLevel(player.getPosition());
    ctx.pushRequest(ModeRequest::wildBattle(species, level));
    return true;
}

bool OverworldMode::warpBlockStarts(GameContext &ctx) {
    Player &player = ctx.world.getPlayer();

    if (!pendingWarpBlock || player.isMoving())
        return false;
    pendingWarpBlock = false;
    ctx.pendingPushBack = true;
    ctx.pushRequest(ModeRequest::dialogue(pendingWarpBlockAction.speaker,
                                          pendingWarpBlockAction.lines, nullptr,
                                          GameState::overworld));
    return true;
}

bool OverworldMode::trainerBattleStarts(GameContext &ctx) {
    Player &player = ctx.world.getPlayer();

    if (player.isMoving())
        return false;

    std::shared_ptr<NPC> npc = ctx.world.NPCSeeingPlayer();
    if (!npc)
        return false;

    if (!dialogueStarts(ctx, npc)) {
        ctx.pushRequest(ModeRequest::trainerBattle(npc));
    }
    return true;
}

bool OverworldMode::dialogueStarts(GameContext &ctx, std::shared_ptr<NPC> npc) {
    Player &player = ctx.world.getPlayer();

    const auto &lines = npc->getDialogueLines(player.getFlags());
    if (lines.empty())
        return false;

    ctx.pushRequest(ModeRequest::dialogue(npc->getName(), lines, npc,
                                          GameState::overworld));
    return true;
}

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
    if (block.type == StoryAction::Type::blockWarp) {
        pendingWarpBlock = true;
        pendingWarpBlockAction = block;
        return;
    }

    ctx.pushRequest(
        ModeRequest::transition(warp->targetMapId, warp->targetPosition));
}

void OverworldMode::handlePlayerInteraction(GameContext &ctx) {
    Player &player = ctx.world.getPlayer();
    Position front = player.getPosition();
    front.moveDirection(player.getFacing());

    std::shared_ptr<NPC> npc =
        ctx.world.findNPCAt(ctx.world.getCurrentMapId(), front);
    if (!npc)
        return;

    if (npc->getType() == NPCType::pc) {
        ctx.playSound(SoundEffect::pcOn);
        ctx.pushRequest(ModeRequest::changeState(GameState::pcBox));
    } else {
        ctx.playSound(SoundEffect::select);
        npc->setFacingOpposite(player.getFacing());
        dialogueStarts(ctx, npc);
    }
}
