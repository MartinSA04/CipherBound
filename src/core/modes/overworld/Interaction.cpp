#include "../../../audio/SoundManager.h"
#include "../../../state/NPC.h"
#include "../../../state/World.h"
#include "../../../state/player/Player.h"
#include "../OverworldMode.h"

void OverworldMode::handlePlayerInteraction(GameContext &ctx) {
    Player &player = ctx.world.getPlayer();
    Position front = player.getPosition();
    front.moveDirection(player.getFacing());

    NPC *npc = ctx.world.findNPCAt(ctx.world.getCurrentMapId(), front);
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
