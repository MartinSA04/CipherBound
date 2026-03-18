#include "../../../state/NPC.h"
#include "../../../state/World.h"
#include "../../../state/player/Player.h"
#include "../../../story/StoryManager.h"
#include "../OverworldMode.h"

bool OverworldMode::wildBattleStarts(GameContext &ctx) {
    Player &player = ctx.world.getPlayer();
    if (player.isMoving())
        return false;

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
    ctx.flow.pendingPushBack = true;
    ctx.pushRequest(ModeRequest::dialogue(pendingWarpBlockAction.speaker,
                                          pendingWarpBlockAction.lines, nullptr,
                                          GameState::overworld));
    return true;
}

bool OverworldMode::trainerBattleStarts(GameContext &ctx) {
    Player &player = ctx.world.getPlayer();

    if (player.isMoving())
        return false;

    NPC *npc = ctx.world.NPCSeeingPlayer();
    if (!npc)
        return false;

    ctx.pushRequest(ModeRequest::trainerBattle(npc, true));
    return true;
}

bool OverworldMode::dialogueStarts(GameContext &ctx, NPC *npc) {
    Player &player = ctx.world.getPlayer();
    std::vector<std::string> lines = npc->getDialogueLines(player.getFlags());

    if (npc->getType() == NPCType::healer) {
        ctx.world.healPlayerParty();
        ctx.world.markHealingCenterUsed(player.getPosition(), player.getFacing());
        ctx.playSound(SoundEffect::recovery);
        if (lines.empty())
            lines = {"Your party is back in shape!"};
    }

    if (lines.empty())
        return false;

    ctx.pushRequest(ModeRequest::dialogue(npc->getName(), lines, npc, GameState::overworld));
    return true;
}
