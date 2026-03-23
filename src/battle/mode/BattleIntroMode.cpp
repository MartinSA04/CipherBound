#include "BattleIntroMode.h"
#include "../../battle/Battle.h"
#include "../../game_data/Pokedex.h"
#include "../../state/NPC.h"
#include "../../state/World.h"
#include "../../state/player/Player.h"
#include "../../ui/GameUI.h"
#include "../../ui/Renderer.h"
#include <algorithm>

BattleIntroMode::BattleIntroMode(int speciesId, int level) : speciesId(speciesId), level(level) {}

BattleIntroMode::BattleIntroMode(NPC *trainer) : trainer(trainer) {}

void BattleIntroMode::update(GameContext &ctx, InputManager & /*input*/) {
    BattlePresentationState &presentation = ctx.battlePresentation();
    presentation.introFrame++;
    if (presentation.introFrame >= BattlePresentationState::introTransitionDuration) {
        // Transition complete — create the Battle object
        if (trainer && trainer->isTrainerType()) {
            ctx.setBattle(std::make_unique<Battle>(ctx.world.getPlayer(), trainer,
                                                   BattleType::trainer, ctx.world.getRng(),
                                                   ctx.pokedex),
                          trainer->getId());

            // Mark all trainer daemons as seen
            for (const auto &d : trainer->getParty())
                ctx.world.getPlayer().markSeen(d.getSpeciesId());
        } else {
            const Species &sp = ctx.pokedex.getSpecies(speciesId);
            auto daemon =
                std::make_unique<Daemon>(Daemon::generateRandomized(sp, level, ctx.world.getRng()));
            ctx.setBattle(std::make_unique<Battle>(ctx.world.getPlayer(), std::move(daemon),
                                                   BattleType::wild, ctx.world.getRng(),
                                                   ctx.pokedex));

            ctx.world.getPlayer().markSeen(speciesId);
        }

        Battle &battle = ctx.battle();
        presentation.beginBattle(battle.getPlayerDaemon().getCurrentHP(),
                                 battle.getOpponentDaemon().getCurrentHP(),
                                 battle.getPlayerDaemon().getExpProgress());
        battle.start();

        ctx.pushRequest(ModeRequest::enterBattleMode());
    }
}

void BattleIntroMode::render(GameContext &ctx) {
    // Draw the overworld underneath the transition effect
    renderOverworld(ctx);

    // Battle intro transition phases (Pokemon-style):
    //   Phase 1 (0-11):    3 quick white flashes over the overworld
    //   Phase 2 (12-49):   Horizontal bars sweep across the screen
    //   Phase 3 (50-89):   Fade from bars to solid black, then hold

    Renderer &renderer = ctx.ui.getRenderer();

    constexpr int FLASH_END = 12;
    constexpr int BARS_END = 50;
    const int frame = ctx.battlePresentation().introFrame;

    if (frame < FLASH_END) {
        // Phase 1: Quick white flashes (3 flashes, 4 frames each at 60fps)
        int flashCycle = frame % 4;
        if (flashCycle < 2) {
            unsigned char a = 200;
            renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                                    TDT4102::Color{255, 255, 255, a});
        }
    } else if (frame < BARS_END) {
        // Phase 2: Horizontal bars sweep in from alternating sides
        int barPhase = frame - FLASH_END;
        int totalBarFrames = BARS_END - FLASH_END;
        int numBars = 8;
        int barHeight = WINDOW_HEIGHT / numBars;

        for (int i = 0; i < numBars; ++i) {
            int barDelay = i * (totalBarFrames / (numBars + 2));
            int barProgress = barPhase - barDelay;
            if (barProgress < 0)
                continue;

            float t = std::min(1.0f, static_cast<float>(barProgress) /
                                         static_cast<float>(totalBarFrames - barDelay));
            int barWidth = static_cast<int>(t * WINDOW_WIDTH);

            int barY = i * barHeight;
            if (i % 2 == 0) {
                renderer.drawFilledRect(0, barY, barWidth, barHeight, TDT4102::Color::black);
            } else {
                renderer.drawFilledRect(WINDOW_WIDTH - barWidth, barY, barWidth, barHeight,
                                        TDT4102::Color::black);
            }
        }
    } else {
        // Phase 3: Solid black (hold until transition completes)
        renderer.drawFilledRect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, TDT4102::Color::black);
    }
}
