#include "../../audio/MusicManager.h"
#include "../Battle.h"
#include "../ui/BattlePresentationState.h"
#include "../../state/World.h"
#include "../../state/player/Player.h"
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"
#include "BattleMode.h"

namespace {

std::string formatStatGainMessage(const BaseStats &gains) {
    return "Stat gains: HP +" + std::to_string(gains.hp) + ", Atk +" +
           std::to_string(gains.attack) + ", Def +" + std::to_string(gains.defense) +
           ", SpA +" + std::to_string(gains.specialAttack) + ", SpD +" +
           std::to_string(gains.specialDefense) + ", Spe +" + std::to_string(gains.speed) + ".";
}

} // namespace

void BattleMode::updateBattleIntroAnim(GameContext &ctx) {
    BattlePresentationState &presentation = ctx.battlePresentation();
    presentation.introFrame++;
    if (presentation.introFrame >= BattlePresentationState::introSceneDuration) {
        ctx.battle().finishIntroAnimation();
        presentation.introFrame = 0;
    }
}

void BattleMode::updateSwitchAnim(GameContext &ctx) {
    Battle &battle = ctx.battle();
    BattlePresentationState &presentation = ctx.battlePresentation();

    if (presentation.switchFrame == 0) {
        if (battle.isSwitchRecalling()) {
            presentation.playerFieldVisible = true;
        } else {
            presentation.playerDisplayHP = battle.getPlayerDaemon().getCurrentHP();
            presentation.playerDisplayEXP = battle.getPlayerDaemon().getExpProgress();
            presentation.playerFieldVisible = false;
            presentation.resetExpAnimation();
        }
    }

    if (presentation.tickSwitchAnimation()) {
        presentation.playerFieldVisible = !battle.isSwitchRecalling();
        battle.finishSwitchAnimation();
    }
}

bool BattleMode::queueDaemonProgression(GameContext &ctx, int partyIndex, bool resolveAllLevels) {
    Daemon &daemon = ctx.world.getPlayer().getDaemon(partyIndex);
    bool queuedAny = false;
    if (partyIndex >= static_cast<int>(progressionLeveledUp.size()))
        progressionLeveledUp.resize(static_cast<std::size_t>(partyIndex + 1), false);

    while (const std::optional<LevelUpResult> levelUp = daemon.resolveLevelUp()) {
        queuedAny = true;
        progressionLeveledUp[static_cast<std::size_t>(partyIndex)] = true;
        progressionEvents.push_back(
            {ProgressionEventType::message, partyIndex, -1, -1, -1,
             daemon.getNickname() + " leveled up to Lv" + std::to_string(levelUp->newLevel) + "!"});
        progressionEvents.push_back(
            {ProgressionEventType::message, partyIndex, -1, -1, -1,
             formatStatGainMessage(levelUp->statGains)});

        for (int moveId : daemon.getMovesLearnedAtLevel(levelUp->newLevel)) {
            if (daemon.knowsMove(moveId))
                continue;

            const MoveData &move = ctx.pokedex.getMove(moveId);
            const int emptySlot = daemon.firstEmptyMoveSlot();
            if (emptySlot >= 0) {
                daemon.learnMove(move.id, emptySlot, move.maxPP);
                progressionEvents.push_back(
                    {ProgressionEventType::message, partyIndex, -1, -1, -1,
                     daemon.getNickname() + " learned " + move.name + "!"});
            } else {
                progressionEvents.push_back(
                    {ProgressionEventType::message, partyIndex, -1, -1, -1,
                     daemon.getNickname() + " wants to learn " + move.name + "!"});
                progressionEvents.push_back(
                    {ProgressionEventType::replaceMove, partyIndex, move.id, -1, -1, {}});
            }
        }

        if (!resolveAllLevels)
            break;
    }

    return queuedAny;
}

bool BattleMode::queueParticipantProgression(GameContext &ctx) {
    Battle &battle = ctx.battle();
    bool queuedAny = false;

    for (int i = 0; i < ctx.world.getPlayer().partySize(); ++i) {
        if (!battle.didPlayerParticipate(i))
            continue;
        if (queueDaemonProgression(ctx, i, true))
            queuedAny = true;

        if (i >= static_cast<int>(progressionLeveledUp.size()) ||
            !progressionLeveledUp[static_cast<std::size_t>(i)]) {
            continue;
        }

        const Daemon &daemon = ctx.world.getPlayer().getDaemon(i);
        const std::optional<int> evolutionTarget = daemon.getEvolutionTargetSpeciesId();
        if (!evolutionTarget.has_value())
            continue;

        progressionEvents.push_back({ProgressionEventType::evolution, i, -1, daemon.getSpeciesId(),
                                     *evolutionTarget, daemon.getNickname()});
        queuedAny = true;
    }

    return queuedAny;
}

bool BattleMode::updateProgressionSequence(GameContext &ctx, InputManager &input) {
    if (progressionEvents.empty()) {
        if (progressionFinishesExpAnimation) {
            progressionFinishesExpAnimation = false;
            ctx.battle().finishEXPAnimation();
            return true;
        }
        return false;
    }

    const ProgressionEvent event = progressionEvents.front();
    if (event.type == ProgressionEventType::message) {
        ctx.ui.setDialogueText(event.text);
        if (ctx.ui.updateTypewriter(input.isConfirmPressed())) {
            ctx.playSound(SoundEffect::select);
            progressionEvents.pop_front();
        }
        return true;
    }

    if (event.type == ProgressionEventType::evolution) {
        static constexpr int evolutionTransformFrame = 420;
        static constexpr int evolutionTotalFrames = 600;
        const std::string finalText =
            event.text + " evolved into " + ctx.pokedex.getSpecies(event.targetSpeciesId).name + "!";

        if (evolutionAnimFrame == 0)
            ctx.music.play(MusicTrack::evolution, ctx.ui.getRenderer().getWindow());
        if (evolutionAnimFrame < evolutionTotalFrames) {
            evolutionAnimFrame++;
            if (!evolutionApplied && evolutionAnimFrame >= evolutionTransformFrame) {
                Daemon &daemon = ctx.world.getPlayer().getDaemon(event.partyIndex);
                const Species &targetSpecies = ctx.pokedex.getSpecies(event.targetSpeciesId);
                daemon.evolveTo(targetSpecies);
                ctx.world.getPlayer().markSeen(targetSpecies.id);
                ctx.world.getPlayer().markCaught(targetSpecies.id);

                if (event.partyIndex == 0)
                    ctx.battlePresentation().playerDisplayHP = daemon.getCurrentHP();

                evolutionApplied = true;
                ctx.playSound(SoundEffect::levelUp);
            }

            if (evolutionAnimFrame == evolutionTotalFrames)
                ctx.music.playOneShot(MusicTrack::evolutionComplete,
                                      ctx.ui.getRenderer().getWindow());
            return true;
        }

        ctx.ui.setDialogueText(finalText);
        if (ctx.ui.updateTypewriter(input.isConfirmPressed())) {
            ctx.playSound(SoundEffect::select);
            progressionEvents.pop_front();
            evolutionAnimFrame = 0;
            evolutionApplied = false;
            if (event.partyIndex >= 0 &&
                event.partyIndex < static_cast<int>(progressionLeveledUp.size())) {
                progressionLeveledUp[static_cast<std::size_t>(event.partyIndex)] = false;
            }
        }
        return true;
    }

    Daemon &daemon = ctx.world.getPlayer().getDaemon(event.partyIndex);
    const MoveData &newMove = ctx.pokedex.getMove(event.moveId);
    ctx.ui.navigateVertical(progressionSelectedMove, 5);

    if (input.isCancelPressed()) {
        progressionSelectedMove = 4;
    }

    if (!input.isConfirmPressed() && !input.isCancelPressed())
        return true;

    ctx.playSound(SoundEffect::select);
    progressionEvents.pop_front();

    if (progressionSelectedMove >= 4) {
        progressionEvents.push_front(
            {ProgressionEventType::message, event.partyIndex, -1, -1, -1,
             daemon.getNickname() + " did not learn " + newMove.name + "."});
        progressionSelectedMove = 0;
        return true;
    }

    const MoveSlot &oldSlot = daemon.getMoves()[static_cast<std::size_t>(progressionSelectedMove)];
    std::string learnedMessage = daemon.getNickname() + " learned " + newMove.name + "!";
    if (oldSlot.moveId >= 0) {
        learnedMessage = daemon.getNickname() + " forgot " +
                         ctx.pokedex.getMove(oldSlot.moveId).name + " and learned " +
                         newMove.name + "!";
    }

    daemon.learnMove(newMove.id, progressionSelectedMove, newMove.maxPP);
    progressionEvents.push_front(
        {ProgressionEventType::message, event.partyIndex, -1, -1, -1, learnedMessage});
    progressionSelectedMove = 0;
    return true;
}

void BattleMode::update(GameContext &ctx, InputManager &input) {
    if (!ctx.hasBattle())
        return;

    Battle &battle = ctx.battle();
    BattlePresentationState &presentation = ctx.battlePresentation();
    BattleState bs = battle.getState();
    if (bs == BattleState::intro) {
        progressionEvents.clear();
        progressionLeveledUp.assign(static_cast<std::size_t>(ctx.world.getPlayer().partySize()),
                                    false);
        progressionFinishesExpAnimation = false;
        progressionSelectedMove = 0;
        evolutionAnimFrame = 0;
        evolutionApplied = false;
    }

    if (updateProgressionSequence(ctx, input))
        return;

    battleAnimFrame++;
    switch (bs) {
    case BattleState::intro:
        updateBattleIntroAnim(ctx);
        return;

    case BattleState::victory:
    case BattleState::defeat:
    case BattleState::fled:
    case BattleState::captured:
        if (bs == BattleState::victory || bs == BattleState::captured) {
            MusicTrack victoryTrack = battle.getType() == BattleType::trainer
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
        if (!battle.isIntroComplete())
            presentation.introFrame++;

        if (ctx.ui.updateTypewriter(input.isConfirmPressed())) {
            ctx.playSound(SoundEffect::select);
            battle.advanceMessage();
            if (battle.getState() == BattleState::intro) {
                presentation.introFrame = 0;
                presentation.introPhase = battle.getIntroPhase();
            }
        }
        return;

    case BattleState::animatingHP: {
        const bool done =
            presentation.tickHPAnimation(battle.getPlayerDaemon().getCurrentHP(),
                                         battle.getOpponentDaemon().getCurrentHP(),
                                         battle.getPlayerDaemon().getMaxHP(),
                                         battle.getOpponentDaemon().getMaxHP());
        if (done)
            battle.finishHPAnimation();
        return;
    }

    case BattleState::animatingEXP: {
        BattleState bps = battle.getPendingState();
        if (bps == BattleState::victory || bps == BattleState::captured) {
            MusicTrack victoryTrack = battle.getType() == BattleType::trainer
                                          ? MusicTrack::trainerVictory
                                          : MusicTrack::wildVictory;
            ctx.music.play(victoryTrack, ctx.ui.getRenderer().getWindow());
        }

        Daemon &daemon = battle.getPlayerDaemon();
        const EXPTickResult result =
            presentation.tickEXPAnimation(daemon.getExpProgress(), daemon.getExpNeeded());

        if (!expSoundPlayed) {
            ctx.playSound(SoundEffect::expTick);
            expSoundPlayed = true;
        }

        if (result == EXPTickResult::filledBar) {
            if (!queueDaemonProgression(ctx, 0, false)) {
                expSoundPlayed = false;
                battle.finishEXPAnimation();
                return;
            }
            ctx.playSound(SoundEffect::levelUp);
            presentation.playerDisplayEXP = 0;
            presentation.resetExpAnimation();
            expSoundPlayed = false;
            presentation.playerDisplayHP = daemon.getCurrentHP();
        } else if (result == EXPTickResult::reachedTarget) {
            ctx.playSound(SoundEffect::expFull);
            expSoundPlayed = false;
            if (queueParticipantProgression(ctx)) {
                progressionFinishesExpAnimation = true;
            } else {
                battle.finishEXPAnimation();
            }
        }
        return;
    }

    case BattleState::opponentTurn:
        battle.executeOpponentTurn();
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
            battle.finishAttackAnimation();
        }
        return;
    }

    case BattleState::animatingSwitch:
        updateSwitchAnim(ctx);
        return;

    case BattleState::choosingAction:
        ctx.ui.navigate2x2(menuSelected);

        if (input.isConfirmPressed()) {
            ctx.playSound(SoundEffect::select);
            BattleAction actions[] = {BattleAction::fight, BattleAction::item,
                                      BattleAction::switchDaemon, BattleAction::flee};
            battle.chooseAction(actions[static_cast<std::size_t>(menuSelected)]);
            moveSelected = 0;
            partySelected = 0;
            bagSelected = 0;
            showingPartyAction = false;
            viewingSummary = false;
            partyActionSelected = 0;
            summaryMoveSelected = 0;
        }
        return;

    case BattleState::choosingMove:
        ctx.ui.navigate2x2(moveSelected);

        if (input.isConfirmPressed()) {
            ctx.playSound(SoundEffect::select);
            battle.chooseMove(moveSelected);
        }
        if (input.isCancelPressed())
            battle.goBack();
        return;

    case BattleState::choosingSwitch:
        if (viewingSummary) {
            Direction dir;
            const bool dirHeld = input.getMovementDirection(dir);
            if (dirHeld && (dir == Direction::left || dir == Direction::right))
                ctx.ui.navigateHorizontal(summaryPage, 3);
            else if (summaryPage == 2)
                ctx.ui.navigateVertical(summaryMoveSelected, 4);
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
                    summaryMoveSelected = 0;
                    viewingSummary = true;
                    showingPartyAction = false;
                    break;
                case 1:
                    showingPartyAction = false;
                    battle.chooseSwitchTarget(partySelected);
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
            battle.goBack();
        return;

    case BattleState::choosingItem: {
        captureAnimDone = false;

        int itemCount = static_cast<int>(ctx.world.getPlayer().getInventory().size());
        if (itemCount == 0) {
            if (input.isCancelPressed())
                battle.goBack();
            return;
        }

        ctx.ui.navigateVertical(bagSelected, itemCount);

        if (input.isConfirmPressed()) {
            if (bagSelected >= 0 && bagSelected < itemCount) {
                ctx.playSound(SoundEffect::select);
                const auto &inv = ctx.world.getPlayer().getInventory();
                battle.chooseItem(inv[static_cast<std::size_t>(bagSelected)].itemId);
                int newSize = static_cast<int>(ctx.world.getPlayer().getInventory().size());
                if (newSize == 0)
                    bagSelected = 0;
                else if (bagSelected >= newSize)
                    bagSelected = newSize - 1;
            }
        }
        if (input.isCancelPressed())
            battle.goBack();
        return;
    }
    }
}
