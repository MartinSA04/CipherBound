#include "BattleMode.h"
#include "../../ui/InputManager.h"
#include "../../ui/GameUI.h"
#include "../../battle/Battle.h"
#include "../../world/World.h"
#include "../../world/Player.h"
#include "../../world/NPC.h"
#include "../../data/Pokedex.h"
#include "../../audio/MusicManager.h"
#include <algorithm>

void BattleMode::setTrainerNPCId(const std::string &id)
{
    currentTrainerNPCId = id;
}

void BattleMode::setTrainer(std::shared_ptr<NPC> trainer)
{
    battleTrainer = std::move(trainer);
}

const std::string &BattleMode::getTrainerNPCId() const
{
    return currentTrainerNPCId;
}

void BattleMode::updateBattleIntroAnim(GameContext &ctx)
{
    ctx.ui.battleIntroFrame++;
    if (ctx.ui.battleIntroFrame >= GameUI::BATTLE_INTRO_SCENE_DURATION)
    {
        ctx.currentBattle->finishIntroAnimation();
        ctx.ui.battleIntroFrame = 0;
    }
}

void BattleMode::update(GameContext &ctx, InputManager &input)
{
    if (!ctx.currentBattle)
        return;

    BattleState bs = ctx.currentBattle->getState();
    switch (bs)
    {
    case BattleState::intro:
        updateBattleIntroAnim(ctx);
        return;

    case BattleState::victory:
    case BattleState::defeat:
    case BattleState::fled:
    case BattleState::captured:
        if (bs == BattleState::victory || bs == BattleState::captured)
        {
            MusicTrack victoryTrack = (battleTrainer && battleTrainer->isTrainerType())
                                          ? MusicTrack::trainerVictory
                                          : MusicTrack::wildVictory;
            ctx.music.play(victoryTrack, ctx.ui.getRenderer().getWindow());
        }
        else if (bs == BattleState::defeat)
        {
            ctx.music.stop();
        }

        if (ctx.ui.updateTypewriter(input.isConfirmPressed()))
            ctx.pushRequest(ModeRequest::endBattle());
        return;

    case BattleState::showingMessages:
        if (!ctx.currentBattle->isIntroComplete())
            ctx.ui.battleIntroFrame++;

        if (ctx.ui.updateTypewriter(input.isConfirmPressed()))
        {
            ctx.currentBattle->advanceMessage();
            if (ctx.currentBattle->getState() == BattleState::intro)
            {
                ctx.ui.battleIntroFrame = 0;
                ctx.ui.battleIntroPhase = ctx.currentBattle->getIntroPhase();
            }
        }
        return;

    case BattleState::animatingHP:
    {
        bool done = ctx.ui.tickHPAnimation(
            ctx.currentBattle->getPlayerCreature().getCurrentHP(),
            ctx.currentBattle->getOpponentCreature().getCurrentHP(),
            ctx.currentBattle->getPlayerCreature().getMaxHP(),
            ctx.currentBattle->getOpponentCreature().getMaxHP());
        if (done)
            ctx.currentBattle->finishHPAnimation();
        return;
    }

    case BattleState::animatingEXP:
    {
        BattleState bps = ctx.currentBattle->getPendingState();
        if (bps == BattleState::victory || bps == BattleState::captured)
        {
            MusicTrack victoryTrack = (battleTrainer && battleTrainer->isTrainerType())
                                          ? MusicTrack::trainerVictory
                                          : MusicTrack::wildVictory;
            ctx.music.play(victoryTrack, ctx.ui.getRenderer().getWindow());
        }

        Creature &creature = ctx.currentBattle->getPlayerCreature();
        EXPTickResult result = ctx.ui.tickEXPAnimation(creature.getExp(), creature.getExpNeeded());

        if (result == EXPTickResult::filledBar && creature.checkLevelUp())
        {
            ctx.ui.playerDisplayEXP = 0;
            ctx.currentBattle->addLevelUpMessage(
                creature.getNickname() + " leveled up to Lv" + std::to_string(creature.getLevel()) + "!");
            ctx.ui.playerDisplayHP = creature.getCurrentHP();
        }
        else if (result == EXPTickResult::reachedTarget)
        {
            ctx.currentBattle->finishEXPAnimation();
        }
        return;
    }

    case BattleState::opponentTurn:
        ctx.currentBattle->executeOpponentTurn();
        return;

    case BattleState::choosingAction:
        ctx.ui.navigate2x2(menuSelected);

        if (input.isConfirmPressed())
        {
            BattleAction actions[] = {
                BattleAction::fight,
                BattleAction::item,
                BattleAction::switchCreature,
                BattleAction::flee};
            ctx.currentBattle->chooseAction(actions[menuSelected]);
            moveSelected = 0;
            partySelected = 0;
            bagSelected = 0;
        }
        return;

    case BattleState::choosingMove:
        ctx.ui.navigate2x2(moveSelected);

        if (input.isConfirmPressed())
            ctx.currentBattle->chooseMove(moveSelected);
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;

    case BattleState::choosingSwitch:
        ctx.ui.navigateVertical(partySelected, ctx.world.getPlayer().partySize());

        if (input.isConfirmPressed())
            ctx.currentBattle->chooseSwitchTarget(partySelected);
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;

    case BattleState::choosingItem:
    {
        int itemCount = static_cast<int>(ctx.world.getPlayer().getInventory().size());
        if (itemCount == 0)
        {
            if (input.isCancelPressed())
                ctx.currentBattle->goBack();
            return;
        }

        ctx.ui.navigateVertical(bagSelected, itemCount);

        if (input.isConfirmPressed())
        {
            if (bagSelected >= 0 && bagSelected < itemCount)
            {
                const auto &inv = ctx.world.getPlayer().getInventory();
                ctx.currentBattle->chooseItem(inv[bagSelected].itemId);
                int newSize = static_cast<int>(ctx.world.getPlayer().getInventory().size());
                if (bagSelected >= newSize)
                    bagSelected = std::max(0, newSize - 1);
            }
        }
        if (input.isCancelPressed())
            ctx.currentBattle->goBack();
        return;
    }
    }
}

void BattleMode::render(GameContext &ctx)
{
    if (!ctx.currentBattle)
        return;

    BattleState bs = ctx.currentBattle->getState();

    // --- Intro animation (no dialogue) ---
    if (bs == BattleState::intro)
    {
        ctx.ui.battleIntroPhase = ctx.currentBattle->getIntroPhase();
        if (ctx.currentBattle->getType() == BattleType::wild)
            ctx.ui.drawBattleIntroScene(&(ctx.currentBattle->getPlayerCreature()),
                                        &(ctx.currentBattle->getOpponentCreature()));
        else
            ctx.ui.drawBattleIntroScene(&(ctx.currentBattle->getPlayerCreature()),
                                        ctx.currentBattle->getOpponent(),
                                        &(ctx.currentBattle->getOpponentCreature()));
        return;
    }

    // --- Intro messages (scene backdrop + dialogue) ---
    if (!ctx.currentBattle->isIntroComplete() && bs == BattleState::showingMessages)
    {
        int savedFrame = ctx.ui.battleIntroFrame;
        ctx.ui.battleIntroFrame = GameUI::BATTLE_INTRO_SCENE_DURATION;
        ctx.ui.battleIntroPhase = ctx.currentBattle->getIntroPhase();
        if (ctx.currentBattle->getType() == BattleType::wild)
            ctx.ui.drawBattleIntroScene(&(ctx.currentBattle->getPlayerCreature()),
                                        &(ctx.currentBattle->getOpponentCreature()));
        else
            ctx.ui.drawBattleIntroScene(&(ctx.currentBattle->getPlayerCreature()),
                                        ctx.currentBattle->getOpponent(),
                                        &(ctx.currentBattle->getOpponentCreature()));
        ctx.ui.battleIntroFrame = savedFrame;
        ctx.ui.drawDialogueBox("", ctx.currentBattle->getMessage());
        return;
    }

    // --- Normal battle rendering ---
    ctx.ui.drawBattleScene(&(ctx.currentBattle->getPlayerCreature()),
                           &(ctx.currentBattle->getOpponentCreature()));

    if (bs == BattleState::choosingAction)
    {
        ctx.ui.drawBattleMenu({"Fight", "Bag", "Pokemon", "Run"}, menuSelected);
    }
    else if (bs == BattleState::choosingMove)
    {
        ctx.ui.drawMoveSelect(ctx.currentBattle->getPlayerCreature(), ctx.pokedex, moveSelected);
    }
    else if (bs == BattleState::choosingSwitch)
    {
        ctx.ui.drawPartyList(ctx.world.getPlayer(), partySelected);
    }
    else if (bs == BattleState::choosingItem)
    {
        ctx.ui.drawBagScreen(ctx.world.getPlayer(), ctx.pokedex, bagSelected);
    }
    else
    {
        ctx.ui.drawDialogueBox("", ctx.currentBattle->getMessage());
    }
}
