#include "../Battle.h"
#include "../ui/BattlePresentationState.h"
#include "../../ui/GameUI.h"
#include "BattleMode.h"

void BattleMode::render(GameContext &ctx) {
    if (!ctx.hasBattle())
        return;

    Battle &battle = ctx.battle();
    BattlePresentationState &presentation = ctx.battlePresentation();
    BattleState bs = battle.getState();

    if (bs == BattleState::intro) {
        presentation.introPhase = battle.getIntroPhase();
        if (battle.getType() == BattleType::wild)
            battleRenderer.drawBattleIntroSceneWild(ctx.ui, battle, presentation);
        else
            battleRenderer.drawBattleIntroSceneTrainer(ctx.ui, battle, presentation);
        return;
    }

    if (!battle.isIntroComplete() && bs == BattleState::showingMessages) {
        const int savedFrame = presentation.introFrame;
        presentation.introFrame = BattlePresentationState::introSceneDuration;
        presentation.introPhase = battle.getIntroPhase();
        if (battle.getType() == BattleType::wild)
            battleRenderer.drawBattleIntroSceneWild(ctx.ui, battle, presentation);
        else
            battleRenderer.drawBattleIntroSceneTrainer(ctx.ui, battle, presentation);
        presentation.introFrame = savedFrame;
        ctx.ui.drawDialogueBox("", battle.getMessage());
        return;
    }

    if (bs == BattleState::animatingCapture) {
        battleRenderer.drawCaptureScene(ctx.ui, battle, presentation, captureAnimFrame,
                                        captureAnimDone);
        return;
    }

    if (bs == BattleState::animatingSwitch) {
        battleRenderer.drawPlayerSwitchScene(ctx.ui, battle, presentation, battleAnimFrame);
        return;
    }

    battleRenderer.drawBattleScene(ctx.ui, battle, presentation, battleAnimFrame, attackAnimFrame,
                                   captureAnimDone);

    if (bs == BattleState::choosingAction) {
        battleRenderer.drawBattleMenu(ctx.ui, menuSelected);
    } else if (bs == BattleState::choosingMove) {
        battleRenderer.drawMoveSelectScreen(ctx.ui, battle.getPlayerDaemon(), ctx.pokedex,
                                            moveSelected);
    } else if (bs == BattleState::choosingSwitch) {
        if (viewingSummary) {
            const Daemon &daemon = ctx.world.getPlayer().getDaemon(partySelected);
            ctx.ui.drawSummaryScreen(daemon, ctx.pokedex, summaryPage);
        } else {
            ctx.ui.drawPartyList(ctx.world.getPlayer(), partySelected);
            if (showingPartyAction) {
                ctx.ui.drawChoiceBox({"Summary", "Switch", "Cancel"}, partyActionSelected);
            }
        }
    } else if (bs == BattleState::choosingItem) {
        ctx.ui.drawBagScreen(ctx.world.getPlayer(), ctx.pokedex, bagSelected);
    } else {
        ctx.ui.drawDialogueBox("", battle.getMessage());
    }
}
