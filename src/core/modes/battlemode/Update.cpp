#include "../BattleMode.h"
#include "../../../audio/MusicManager.h"
#include "../../../state/World.h"
#include "../../../state/player/Player.h"
#include "../../../ui/GameUI.h"
#include "../../../ui/InputManager.h"

void BattleMode::updateBattleIntroAnim(GameContext &ctx) {
    ctx.ui.battleIntroFrame++;
    if (ctx.ui.battleIntroFrame >= GameUI::BATTLE_INTRO_SCENE_DURATION) {
        ctx.currentBattle->finishIntroAnimation();
        ctx.ui.battleIntroFrame = 0;
    }
}

void BattleMode::update(GameContext &ctx, InputManager &input) {
    if (!ctx.currentBattle)
        return;

    battleAnimFrame++;

    BattleState bs = ctx.currentBattle->getState();
    switch (bs) {
    case BattleState::intro:
        updateBattleIntroAnim(ctx);
        return;

    case BattleState::victory:
    case BattleState::defeat:
    case BattleState::fled:
    case BattleState::captured:
        if (bs == BattleState::victory || bs == BattleState::captured) {
            MusicTrack victoryTrack = (battleTrainer && battleTrainer->isTrainerType())
                                          ? MusicTrack::trainerVictory
                                          : MusicTrack::wildVictory;
            ctx.music.play(victoryTrack, ctx.ui.getRenderer().getWindow());
        } else if (bs == BattleState::defeat) {
            ctx.music.stop();
        }

        if (ctx.ui.updateTypewriter(input.isConfirmPressed())) {
            ctx.playSound(SoundEffect::select);
            ctx.pushRequest(ModeRequest::endBattle());
        }
        return;

    case BattleState::showingMessages:
        if (!ctx.currentBattle->isIntroComplete())
            ctx.ui.battleIntroFrame++;

        if (ctx.ui.updateTypewriter(input.isConfirmPressed())) {
            ctx.playSound(SoundEffect::select);
            ctx.currentBattle->advanceMessage();
            if (ctx.currentBattle->getState() == BattleState::intro) {
                ctx.ui.battleIntroFrame = 0;
                ctx.ui.battleIntroPhase = ctx.currentBattle->getIntroPhase();
            }
        }
        return;

    case BattleState::animatingHP: {
        bool done = ctx.ui.tickHPAnimation(ctx.currentBattle->getPlayerDaemon().getCurrentHP(),
                                           ctx.currentBattle->getOpponentDaemon().getCurrentHP(),
                                           ctx.currentBattle->getPlayerDaemon().getMaxHP(),
                                           ctx.currentBattle->getOpponentDaemon().getMaxHP());
        if (done)
            ctx.currentBattle->finishHPAnimation();
        return;
    }

    case BattleState::animatingEXP: {
        BattleState bps = ctx.currentBattle->getPendingState();
        if (bps == BattleState::victory || bps == BattleState::captured) {
            MusicTrack victoryTrack = (battleTrainer && battleTrainer->isTrainerType())
                                          ? MusicTrack::trainerVictory
                                          : MusicTrack::wildVictory;
            ctx.music.play(victoryTrack, ctx.ui.getRenderer().getWindow());
        }

        Daemon &daemon = ctx.currentBattle->getPlayerDaemon();
        EXPTickResult result = ctx.ui.tickEXPAnimation(daemon.getExp(), daemon.getExpNeeded());

        if (!expSoundPlayed) {
            ctx.playSound(SoundEffect::expTick);
            expSoundPlayed = true;
        }

        if (result == EXPTickResult::filledBar && daemon.checkLevelUp()) {
            ctx.playSound(SoundEffect::levelUp);
            ctx.ui.playerDisplayEXP = 0;
            ctx.ui.expAnimFrame = 0;
            ctx.ui.expAnimStartEXP = -1;
            expSoundPlayed = false;
            ctx.currentBattle->addLevelUpMessage(daemon.getNickname() + " leveled up to Lv" +
                                                 std::to_string(daemon.getLevel()) + "!");
            ctx.ui.playerDisplayHP = daemon.getCurrentHP();
        } else if (result == EXPTickResult::reachedTarget) {
            ctx.playSound(SoundEffect::expFull);
            expSoundPlayed = false;
            ctx.currentBattle->finishEXPAnimation();
        }
        return;
    }

    case BattleState::opponentTurn:
        ctx.currentBattle->executeOpponentTurn();
        return;

    case BattleState::animatingCapture:
        updateCaptureAnim(ctx);
        return;

    case BattleState::animatingAttack: {
        static constexpr int ATTACK_TOTAL_FRAMES = 36;
        attackAnimFrame++;
        if (attackAnimFrame == 6)
            ctx.playSound(SoundEffect::attack);
        if (attackAnimFrame >= ATTACK_TOTAL_FRAMES) {
            attackAnimFrame = 0;
            ctx.currentBattle->finishAttackAnimation();
        }
        return;
    }

    case BattleState::choosingAction:
        ctx.ui.navigate2x2(menuSelected);

        if (input.isConfirmPressed()) {
            ctx.playSound(SoundEffect::select);
            BattleAction actions[] = {BattleAction::fight, BattleAction::item,
                                      BattleAction::switchDaemon, BattleAction::flee};
            ctx.currentBattle->chooseAction(actions[static_cast<std::size_t>(menuSelected)]);
            moveSelected = 0;
            partySelected = 0;
            bagSelected = 0;
            showingPartyAction = false;
            viewingSummary = false;
            partyActionSelected = 0;
        }
        return;

    case BattleState::choosingMove:
        ctx.ui.navigate2x2(moveSelected);

        if (input.isConfirmPressed()) {
            ctx.playSound(SoundEffect::select);
            ctx.currentBattle->chooseMove(moveSelected);
        }
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;

    case BattleState::choosingSwitch:
        if (viewingSummary) {
            Direction dir;
            bool dirHeld = input.getMovementDirection(dir);
            if (dirHeld) {
                if (dir == Direction::left && summaryPage > 0)
                    summaryPage = 0;
                else if (dir == Direction::right && summaryPage < 1)
                    summaryPage = 1;
            }
            if (input.isCancelPressed()) {
                viewingSummary = false;
                showingPartyAction = true;
            }
            return;
        }

        if (showingPartyAction) {
            ctx.ui.navigateVertical(partyActionSelected, 3);

            if (input.isConfirmPressed()) {
                ctx.playSound(SoundEffect::select);
                switch (partyActionSelected) {
                case 0:
                    summaryPage = 0;
                    viewingSummary = true;
                    showingPartyAction = false;
                    break;
                case 1:
                    showingPartyAction = false;
                    ctx.currentBattle->chooseSwitchTarget(partySelected);
                    break;
                case 2:
                    showingPartyAction = false;
                    break;
                }
            }
            if (input.isCancelPressed()) {
                showingPartyAction = false;
            }
            return;
        }

        ctx.ui.navigateVertical(partySelected, ctx.world.getPlayer().partySize());

        if (input.isConfirmPressed()) {
            ctx.playSound(SoundEffect::select);
            showingPartyAction = true;
            partyActionSelected = 0;
        }
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;

    case BattleState::choosingItem: {
        captureAnimDone = false;

        int itemCount = static_cast<int>(ctx.world.getPlayer().getInventory().size());
        if (itemCount == 0) {
            if (input.isCancelPressed())
                ctx.currentBattle->goBack();
            return;
        }

        ctx.ui.navigateVertical(bagSelected, itemCount);

        if (input.isConfirmPressed()) {
            if (bagSelected >= 0 && bagSelected < itemCount) {
                ctx.playSound(SoundEffect::select);
                const auto &inv = ctx.world.getPlayer().getInventory();
                ctx.currentBattle->chooseItem(inv[static_cast<std::size_t>(bagSelected)].itemId);
                int newSize = static_cast<int>(ctx.world.getPlayer().getInventory().size());
                if (newSize == 0)
                    bagSelected = 0;
                else if (bagSelected >= newSize)
                    bagSelected = newSize - 1;
            }
        }
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;
    }
    }
}
