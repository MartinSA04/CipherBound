#include "SessionCoordinator.h"
#include "../audio/MusicManager.h"
#include "../battle/Battle.h"
#include "../battle/TrainerApproachCutscene.h"
#include "../battle/mode/BattleIntroMode.h"
#include "../battle/mode/BattleMode.h"
#include "../common/StringUtils.h"
#include "../common/VariantUtils.h"
#include "../cutscene/CutsceneRunner.h"
#include "../ui/GameUI.h"
#include "modes/BagMode.h"
#include "modes/CutSceneMode.h"
#include "modes/DaemonNamingMode.h"
#include "modes/DaemondexMode.h"
#include "modes/DialogueChoiceMode.h"
#include "modes/DialogueMode.h"
#include "modes/MenuMode.h"
#include "modes/OverworldMode.h"
#include "modes/PCBoxMode.h"
#include "modes/PartyMode.h"
#include "modes/PlayerStatsMode.h"
#include "modes/SaveMode.h"
#include "modes/ShopMode.h"
#include "modes/TitleScreenMode.h"
#include "modes/TransitionMode.h"
#include <iostream>
#include <stdexcept>

namespace {

bool isAdjacentToPlayer(const NPC &trainer, const Player &player) {
    const Position trainerPosition = trainer.getPosition();
    const Position playerPosition = player.getPosition();
    const int distance = std::abs(trainerPosition.x - playerPosition.x) +
                         std::abs(trainerPosition.y - playerPosition.y);
    return distance == 1;
}

std::string resolveDialogueSpeaker(const Player &player, const std::string &speaker) {
    return StringUtils::substitutePlayerName(speaker, player.getName());
}

std::vector<std::string> resolveDialogueLines(const Player &player,
                                              const std::vector<std::string> &lines) {
    return StringUtils::substitutePlayerName(lines, player.getName());
}

} // namespace

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
    constexpr int maxRequestPasses = 16;

    for (int pass = 0; pass < maxRequestPasses; ++pass) {
        std::vector<ModeRequest> requests = ctx.mailbox.drain();
        if (requests.empty())
            return;

        for (auto &req : requests) {
            std::visit(
                VariantUtils::Overloaded{
                    [this](const ChangeStateRequest &request) { handleRequest(request); },
                    [this](const EnterBattleModeRequest &request) { handleRequest(request); },
                    [this](const StartWildBattleRequest &request) { handleRequest(request); },
                    [this](const StartTrainerBattleRequest &request) { handleRequest(request); },
                    [this](const StartTrainerBattleIntroRequest &request) {
                        handleRequest(request);
                    },
                    [this](const EndBattleRequest &request) { handleRequest(request); },
                    [this](const TransitionToMapRequest &request) { handleRequest(request); },
                    [this](const StartDialogueRequest &request) { handleRequest(request); },
                    [this](const StartDialogueChoiceRequest &request) { handleRequest(request); },
                    [this](const StartDaemonNamingRequest &request) { handleRequest(request); },
                    [this](const OpenShopRequest &request) { handleRequest(request); },
                    [this](const StartCutsceneRequest &request) { handleRequest(request); },
                    [this](const StoryActionRequest &request) { handleRequest(request); }},
                req.payload);
        }
    }

    if (!ctx.mailbox.pending.empty())
        std::cerr << "SessionCoordinator: request processing exceeded pass limit\n";
}

void SessionCoordinator::switchMode(GameState newState) {
    switchToMode(newState, createMode(newState));
}

void SessionCoordinator::handleRequest(const ChangeStateRequest &req) {
    switchMode(req.targetState);
}

void SessionCoordinator::handleRequest(const EnterBattleModeRequest & /*req*/) {
    switchToMode(GameState::battle, std::make_unique<BattleMode>());
}

void SessionCoordinator::handleRequest(const StartWildBattleRequest &req) {
    ctx.resetBattlePresentation();
    switchToMode(GameState::battleIntro,
                 std::make_unique<BattleIntroMode>(req.speciesId, req.level));
    ctx.playMusic(MusicTrack::wildBattle);
}

void SessionCoordinator::handleRequest(const StartTrainerBattleRequest &req) {
    if (req.npc == nullptr)
        return;

    if (!req.includePreBattleDialogue && isAdjacentToPlayer(*req.npc, ctx.world.getPlayer())) {
        handleRequest(StartTrainerBattleIntroRequest{req.npc});
        return;
    }

    if (req.includePreBattleDialogue)
        ctx.playMusicOneShot(MusicTrack::trainerEyesMeet);

    ctx.flow.cutsceneEndRequest = ModeRequest::trainerBattleIntro(req.npc);

    if (ctx.cutsceneRunner.load(TrainerApproachCutscene::build(*req.npc, ctx.world.getPlayer(),
                                                               req.includePreBattleDialogue))) {
        ctx.cutsceneRunner.start();
        switchMode(GameState::cutscene);
    } else {
        ctx.flow.cutsceneEndRequest.reset();
        handleRequest(StartTrainerBattleIntroRequest{req.npc});
    }
}

void SessionCoordinator::handleRequest(const StartTrainerBattleIntroRequest &req) {
    ctx.resetBattlePresentation();
    switchToMode(GameState::battleIntro, std::make_unique<BattleIntroMode>(req.npc));
    ctx.playMusic(MusicTrack::trainerBattle);
}

void SessionCoordinator::handleRequest(const EndBattleRequest & /*req*/) {
    bool playerBlackedOut = false;
    if (ctx.hasBattle()) {
        playerBlackedOut = ctx.battle().getState() == BattleState::defeat;
        if (playerBlackedOut) {
            if (NPC *trainer = ctx.battle().getOpponent(); trainer != nullptr)
                trainer->fullHealParty();
        }
        if (!ctx.battleTrainerNPCId().empty()) {
            BattleResult result = ctx.battle().getResult();
            if (result.playerWon)
                ctx.world.setNPCDefeated(ctx.battleTrainerNPCId());
        }
    }

    if (playerBlackedOut) {
        switchToMode(GameState::transition,
                     std::make_unique<TransitionMode>(TransitionMode::Kind::blackoutRespawn));
        return;
    }

    ctx.clearBattle();
    switchMode(GameState::overworld);

    MusicTrack mapTrack = MusicManager::trackForMap(ctx.world.getCurrentMapId());
    ctx.playMusic(mapTrack);
}

void SessionCoordinator::handleRequest(const TransitionToMapRequest &req) {
    const std::string &mapId = ctx.world.getCurrentMapId();
    ctx.world.getMap(mapId).setOccupied(ctx.world.getPlayer().getPosition(), false);
    switchToMode(GameState::transition, std::make_unique<TransitionMode>(req.mapId, req.spawn));
}

void SessionCoordinator::handleRequest(const StartDialogueRequest &req) {
    ctx.flow.dialogueReturnState = req.returnState;
    switchToMode(
        GameState::dialogue,
        std::make_unique<DialogueMode>(resolveDialogueSpeaker(ctx.world.getPlayer(), req.speaker),
                                       resolveDialogueLines(ctx.world.getPlayer(), req.lines),
                                       req.npc, req.returnState));
}

void SessionCoordinator::handleRequest(const StartDialogueChoiceRequest &req) {
    switchToMode(
        GameState::dialogueChoice,
        std::make_unique<DialogueChoiceMode>(
            StringUtils::substitutePlayerName(req.choiceOptions, ctx.world.getPlayer().getName()),
            req.choiceContext, ctx.flow.dialogueReturnState));
}

void SessionCoordinator::handleRequest(const StartDaemonNamingRequest &req) {
    switchToMode(GameState::daemonNaming,
                 std::make_unique<DaemonNamingMode>(req.daemon, req.purpose,
                                                    resolveDialogueSpeaker(ctx.world.getPlayer(),
                                                                          req.completionSpeaker),
                                                    resolveDialogueLines(ctx.world.getPlayer(),
                                                                         req.completionLines),
                                                    req.returnState));
}

void SessionCoordinator::handleRequest(const OpenShopRequest &req) {
    switchToMode(GameState::shop,
                 std::make_unique<ShopMode>(req.title, req.shopkeeperName, req.itemIds));
}

void SessionCoordinator::handleRequest(const StartCutsceneRequest &req) {
    ctx.flow.cutsceneEndRequest.reset();
    if (ctx.cutsceneRunner.load(req.cutscenePath)) {
        ctx.cutsceneRunner.start();
        switchMode(GameState::cutscene);
    } else {
        std::cerr << "SessionCoordinator: failed to load cutscene '" << req.cutscenePath << "'\n";
    }
}

void SessionCoordinator::handleRequest(const StoryActionRequest &req) {
    std::visit(VariantUtils::Overloaded{
                   [this](const StoryNoAction &action) { handleStoryAction(action); },
                   [this](const StoryBlockWarpAction &action) { handleStoryAction(action); },
                   [this](const StoryShowChoiceAction &action) { handleStoryAction(action); },
                   [this](const StoryStartBattleAction &action) { handleStoryAction(action); },
                   [this](const StoryShowDialogueAction &action) { handleStoryAction(action); },
                   [this](const StoryPromptStarterNicknameAction &action) {
                       handleStoryAction(action);
                   },
                   [this](const StoryReturnToStateAction &action) { handleStoryAction(action); },
                   [this](const StoryStartCutsceneAction &action) { handleStoryAction(action); }},
               req.action.payload);
}

void SessionCoordinator::handleStoryAction(const StoryNoAction & /*action*/) {}

void SessionCoordinator::handleStoryAction(const StoryBlockWarpAction &action) {
    ctx.flow.pendingPushBack = true;
    handleRequest(
        StartDialogueRequest{action.speaker, action.lines, nullptr, GameState::overworld});
}

void SessionCoordinator::handleStoryAction(const StoryShowChoiceAction &action) {
    handleRequest(StartDialogueChoiceRequest{action.options, action.choiceContext,
                                             ctx.flow.dialogueReturnState});
}

void SessionCoordinator::handleStoryAction(const StoryStartBattleAction &action) {
    handleRequest(StartTrainerBattleRequest{action.trainer});
}

void SessionCoordinator::handleStoryAction(const StoryShowDialogueAction &action) {
    handleRequest(
        StartDialogueRequest{action.speaker, action.lines, nullptr, GameState::overworld});
}

void SessionCoordinator::handleStoryAction(const StoryPromptStarterNicknameAction &action) {
    handleRequest(StartDaemonNamingRequest{action.daemon, DaemonNamingPurpose::starter,
                                           action.speaker, action.lines, GameState::overworld});
}

void SessionCoordinator::handleStoryAction(const StoryReturnToStateAction & /*action*/) {
    if (ctx.flow.pendingPushBack) {
        ctx.flow.pendingPushBack = false;
        ctx.world.pushPlayerBack();
    }
    switchMode(ctx.flow.dialogueReturnState);
}

void SessionCoordinator::handleStoryAction(const StoryStartCutsceneAction &action) {
    handleRequest(StartCutsceneRequest{action.cutscenePath});
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
    case GameState::playerStats:
        return std::make_unique<PlayerStatsMode>();
    case GameState::party:
        return std::make_unique<PartyMode>();
    case GameState::bag:
        return std::make_unique<BagMode>();
    case GameState::shop:
        return std::make_unique<ShopMode>();
    case GameState::saving:
        return std::make_unique<SaveMode>();
    case GameState::pcBox:
        return std::make_unique<PCBoxMode>();
    case GameState::cutscene:
        return std::make_unique<CutSceneMode>();
    case GameState::daemondex:
        return std::make_unique<DaemondexMode>();
    case GameState::daemonNaming:
        throw std::logic_error("Daemon naming mode requires explicit request data");
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
    case GameState::playerStats:
        return ScreenType::menu;
    case GameState::party:
        return ScreenType::party;
    case GameState::bag:
        return ScreenType::bag;
    case GameState::shop:
        return ScreenType::shop;
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
    case GameState::daemonNaming:
        return ScreenType::dialogue;
    default:
        return ScreenType::overworld;
    }
}
