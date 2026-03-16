#include "SessionCoordinator.h"
#include "../audio/MusicManager.h"
#include "../battle/Battle.h"
#include "../ui/GameUI.h"
#include "CutsceneRunner.h"
#include "modes/BagMode.h"
#include "modes/BattleIntroMode.h"
#include "modes/BattleMode.h"
#include "modes/CutSceneMode.h"
#include "modes/DaemondexMode.h"
#include "modes/DialogueChoiceMode.h"
#include "modes/DialogueMode.h"
#include "modes/MenuMode.h"
#include "modes/OverworldMode.h"
#include "modes/PCBoxMode.h"
#include "modes/PartyMode.h"
#include "modes/SaveMode.h"
#include "modes/TitleScreenMode.h"
#include "modes/TransitionMode.h"

SessionCoordinator::SessionCoordinator(GameContext &ctx) : ctx(ctx) {}

void SessionCoordinator::update(InputManager &input) {
    if (currentMode)
        currentMode->update(ctx, input);
}

void SessionCoordinator::render() {
    if (currentMode)
        currentMode->render(ctx);
}

void SessionCoordinator::processRequests() {
    // Drain all pending requests (a request may push more, but we process them
    // in order and stop after one pass to avoid infinite loops)
    std::vector<ModeRequest> requests = ctx.mailbox.drain();

    for (auto &req : requests) {
        switch (req.type) {
        case ModeRequest::Type::changeState:
            handleChangeStateRequest(req);
            break;

        case ModeRequest::Type::startWildBattle:
            handleStartWildBattleRequest(req);
            break;

        case ModeRequest::Type::startTrainerBattle:
            handleStartTrainerBattleRequest(req);
            break;

        case ModeRequest::Type::endBattle:
            handleEndBattleRequest();
            break;

        case ModeRequest::Type::transitionToMap:
            handleTransitionToMapRequest(req);
            break;

        case ModeRequest::Type::startDialogue:
            handleStartDialogueRequest(req);
            break;

        case ModeRequest::Type::startDialogueChoice:
            handleStartDialogueChoiceRequest(req);
            break;

        case ModeRequest::Type::startCutscene:
            handleStartCutsceneRequest(req);
            break;

        case ModeRequest::Type::handleStoryAction:
            handleStoryAction(req.storyActionData);
            break;
        }
    }
}

void SessionCoordinator::switchMode(GameState newState) {
    switchToMode(newState, createMode(newState));
}

void SessionCoordinator::handleChangeStateRequest(const ModeRequest &req) {
    if (req.targetState == GameState::battle) {
        auto mode = std::make_unique<BattleMode>();
        if (req.npc) {
            mode->setTrainerNPCId(req.npc->getId());
            mode->setTrainer(req.npc);
        }
        switchToMode(GameState::battle, std::move(mode));
        return;
    }

    switchMode(req.targetState);
}

void SessionCoordinator::handleStartWildBattleRequest(const ModeRequest &req) {
    ctx.ui.battleIntroFrame = 0;
    switchToMode(GameState::battleIntro,
                 std::make_unique<BattleIntroMode>(req.speciesId, req.level));
    ctx.music.play(MusicTrack::wildBattle, ctx.ui.getRenderer().getWindow());
}

void SessionCoordinator::handleStartTrainerBattleRequest(const ModeRequest &req) {
    ctx.ui.battleIntroFrame = 0;
    switchToMode(GameState::battleIntro, std::make_unique<BattleIntroMode>(req.npc));
    ctx.music.play(MusicTrack::trainerBattle, ctx.ui.getRenderer().getWindow());
}

void SessionCoordinator::handleEndBattleRequest() {
    if (ctx.hasBattle()) {
        auto *battleMode = dynamic_cast<BattleMode *>(currentMode.get());
        if (battleMode && !battleMode->getTrainerNPCId().empty()) {
            BattleResult result = ctx.battle().getResult();
            if (result.playerWon)
                ctx.world.setNPCDefeated(battleMode->getTrainerNPCId());
        }
    }

    ctx.clearBattle();
    switchMode(GameState::overworld);

    MusicTrack mapTrack = MusicManager::trackForMap(ctx.world.getCurrentMapId());
    ctx.music.play(mapTrack, ctx.ui.getRenderer().getWindow());
}

void SessionCoordinator::handleTransitionToMapRequest(const ModeRequest &req) {
    const std::string &mapId = ctx.world.getCurrentMapId();
    ctx.world.getMap(mapId).setOccupied(ctx.world.getPlayer().getPosition(), false);
    switchToMode(GameState::transition, std::make_unique<TransitionMode>(req.mapId, req.spawn));
}

void SessionCoordinator::handleStartDialogueRequest(const ModeRequest &req) {
    ctx.flow.dialogueReturnState = req.returnState;
    switchToMode(GameState::dialogue,
                 std::make_unique<DialogueMode>(req.speaker, req.lines, req.npc, req.returnState));
}

void SessionCoordinator::handleStartDialogueChoiceRequest(const ModeRequest &req) {
    switchToMode(GameState::dialogueChoice,
                 std::make_unique<DialogueChoiceMode>(req.choiceOptions, req.choiceContext,
                                                      ctx.flow.dialogueReturnState));
}

void SessionCoordinator::handleStartCutsceneRequest(const ModeRequest &req) {
    if (ctx.cutsceneRunner.load(req.cutscenePath)) {
        ctx.cutsceneRunner.start();
        switchMode(GameState::cutscene);
    }
}

void SessionCoordinator::handleStoryAction(const StoryAction &action) {
    switch (action.type) {
    case StoryAction::Type::startBattle:
        ctx.pushRequest(ModeRequest::trainerBattle(action.trainer));
        break;

    case StoryAction::Type::showChoice:
        ctx.pushRequest(ModeRequest::dialogueChoice(action.options, action.choiceContext,
                                                    ctx.flow.dialogueReturnState));
        break;

    case StoryAction::Type::showDialogue:
        ctx.pushRequest(
            ModeRequest::dialogue(action.speaker, action.lines, nullptr, GameState::overworld));
        break;

    case StoryAction::Type::startCutscene:
        ctx.pushRequest(ModeRequest::cutscene(action.cutscenePath));
        break;

    case StoryAction::Type::returnToState:
    default:
        if (ctx.flow.pendingPushBack) {
            ctx.flow.pendingPushBack = false;
            ctx.world.pushPlayerBack();
        }
        switchMode(ctx.flow.dialogueReturnState);
        break;
    }
}

void SessionCoordinator::switchToMode(GameState newState, std::unique_ptr<GameMode> mode) {
    if (currentMode)
        currentMode->onExit(ctx);

    currentMode = std::move(mode);
    currentState = newState;
    ctx.ui.setScreen(screenForState(newState));

    if (currentMode)
        currentMode->onEnter(ctx);
}

std::unique_ptr<GameMode> SessionCoordinator::createMode(GameState state) {
    switch (state) {
    case GameState::titleScreen:
        return std::make_unique<TitleScreenMode>();
    case GameState::overworld:
        return std::make_unique<OverworldMode>();
    case GameState::battle:
        return std::make_unique<BattleMode>();
    case GameState::menu:
        return std::make_unique<MenuMode>();
    case GameState::party:
        return std::make_unique<PartyMode>();
    case GameState::bag:
        return std::make_unique<BagMode>();
    case GameState::saving:
        return std::make_unique<SaveMode>();
    case GameState::pcBox:
        return std::make_unique<PCBoxMode>();
    case GameState::cutscene:
        return std::make_unique<CutSceneMode>();
    case GameState::daemondex:
        return std::make_unique<DaemondexMode>();
    default:
        return std::make_unique<OverworldMode>();
    }
}

ScreenType SessionCoordinator::screenForState(GameState gs) {
    switch (gs) {
    case GameState::titleScreen:
        return ScreenType::title;
    case GameState::overworld:
        return ScreenType::overworld;
    case GameState::battle:
        return ScreenType::battle;
    case GameState::menu:
        return ScreenType::menu;
    case GameState::party:
        return ScreenType::party;
    case GameState::bag:
        return ScreenType::bag;
    case GameState::saving:
        return ScreenType::dialogue;
    case GameState::dialogue:
        return ScreenType::dialogue;
    case GameState::dialogueChoice:
        return ScreenType::dialogue;
    case GameState::transition:
        return ScreenType::overworld;
    case GameState::battleIntro:
        return ScreenType::overworld;
    case GameState::pcBox:
        return ScreenType::menu;
    case GameState::cutscene:
        return ScreenType::overworld;
    case GameState::daemondex:
        return ScreenType::menu;
    default:
        return ScreenType::overworld;
    }
}
