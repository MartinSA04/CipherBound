#include "BattleIntroMode.h"
#include "../../ui/GameUI.h"
#include "../../battle/Battle.h"
#include "../../world/World.h"
#include "../../world/Player.h"
#include "../../world/NPC.h"
#include "../../data/Pokedex.h"

BattleIntroMode::BattleIntroMode(int speciesId, int level)
    : speciesId(speciesId), level(level) {}

BattleIntroMode::BattleIntroMode(std::shared_ptr<NPC> trainer)
    : trainer(std::move(trainer)) {}

void BattleIntroMode::update(GameContext &ctx, InputManager & /*input*/)
{
    ctx.ui.battleIntroFrame++;
    if (ctx.ui.battleIntroFrame >= GameUI::BATTLE_INTRO_DURATION)
    {
        // Transition complete — create the Battle object
        if (trainer && trainer->isTrainerType())
        {
            ctx.currentBattle = std::make_unique<Battle>(
                ctx.world.getPlayer(), trainer,
                BattleType::trainer, ctx.world.getRng(), ctx.pokedex);
        }
        else
        {
            const Species &sp = ctx.pokedex.getSpecies(speciesId);
            auto creature = std::make_unique<Creature>(sp, level);
            ctx.currentBattle = std::make_unique<Battle>(
                ctx.world.getPlayer(), std::move(creature),
                BattleType::wild, ctx.world.getRng(), ctx.pokedex);
        }

        ctx.ui.playerDisplayHP = ctx.currentBattle->getPlayerCreature().getCurrentHP();
        ctx.ui.opponentDisplayHP = ctx.currentBattle->getOpponentCreature().getCurrentHP();
        ctx.ui.playerDisplayEXP = ctx.currentBattle->getPlayerCreature().getExp();
        ctx.currentBattle->start();
        ctx.ui.battleIntroPhase = 0;
        ctx.ui.battleIntroFrame = 0;

        // Request transition to battle mode — Session will set up BattleMode with trainer info
        ModeRequest req = ModeRequest::changeState(GameState::battle);
        req.npc = trainer; // pass trainer info along
        ctx.pushRequest(std::move(req));
    }
}

void BattleIntroMode::render(GameContext &ctx)
{
    // Draw the overworld underneath the transition effect
    renderOverworld(ctx);
    ctx.ui.drawBattleIntro();
}
