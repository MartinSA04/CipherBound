#include "Session.h"
#include "../battle/Battle.h"
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
#include <thread>

Session::Session(int seed)
    : world(seed), pokedex(), ui(), saveManager(), story(), music(), sound(), cutsceneRunner(),
      ctx{world, pokedex, ui, saveManager, story, music, cutsceneRunner, sound, nullptr, -1, false, {}} {}

// ── Main loop
// ──────────────────────────────────────────────────────────────────

void Session::run() {
    while (!ui.shouldClose()) {
        tick();
    }
}

void Session::tick() {
#ifndef __EMSCRIPTEN__
    auto frameStart = std::chrono::high_resolution_clock::now();
#endif

    ui.beginFrame();

    // Update active mode
    if (currentMode)
        currentMode->update(ctx, ui.getInput());

    // Process any transition requests the mode pushed
    processRequests();

    // Render active mode
    if (currentMode)
        currentMode->render(ctx);

    ui.updateInput();
    ui.endFrame();

#ifndef __EMSCRIPTEN__
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double>(frameEnd - frameStart);
    if (elapsed < targetFrameTime)
        std::this_thread::sleep_for(targetFrameTime - elapsed);
#endif
}

// ── Initialisation
// ─────────────────────────────────────────────────────────────

void Session::init() {
    pokedex.loadSpecies("assets/data/species.txt");
    pokedex.loadMoves("assets/data/moves.txt");
    pokedex.loadItems("assets/data/items.txt");

    ui.getOverworldRenderer().loadSprites();
    world.generate(pokedex);
    ui.getOverworldRenderer().loadMapBackgrounds(world);
    music.loadAll();
    sound.loadAll();

    switchMode(GameState::titleScreen);
    music.play(MusicTrack::titleScreen, ui.getRenderer().getWindow());
}

// ── Request processing (cross-mode orchestration)
// ──────────────────────────────

void Session::processRequests() {
    // Drain all pending requests (a request may push more, but we process them
    // in order and stop after one pass to avoid infinite loops)
    std::vector<ModeRequest> requests;
    std::swap(requests, ctx.pendingRequests);

    for (auto &req : requests) {
        switch (req.type) {
        case ModeRequest::Type::changeState: {
            if (req.targetState == GameState::battle) {
                // Special case: set up BattleMode with trainer info from
                // BattleIntroMode
                auto mode = std::make_unique<BattleMode>();
                if (req.npc) {
                    mode->setTrainerNPCId(req.npc->getId());
                    mode->setTrainer(req.npc);
                }
                switchToMode(GameState::battle, std::move(mode));
            } else {
                switchMode(req.targetState);
            }
            break;
        }

        case ModeRequest::Type::startWildBattle: {
            ctx.ui.battleIntroFrame = 0;
            switchToMode(GameState::battleIntro, std::make_unique<BattleIntroMode>(req.speciesId, req.level));
            music.play(MusicTrack::wildBattle, ui.getRenderer().getWindow());
            break;
        }

        case ModeRequest::Type::startTrainerBattle: {
            ctx.ui.battleIntroFrame = 0;
            switchToMode(GameState::battleIntro, std::make_unique<BattleIntroMode>(req.npc));
            music.play(MusicTrack::trainerBattle, ui.getRenderer().getWindow());
            break;
        }

        case ModeRequest::Type::endBattle: {
            if (ctx.currentBattle) {
                auto *battleMode = dynamic_cast<BattleMode *>(currentMode.get());
                if (battleMode && !battleMode->getTrainerNPCId().empty()) {
                    BattleResult result = ctx.currentBattle->getResult();
                    if (result.playerWon)
                        world.setNPCDefeated(battleMode->getTrainerNPCId());
                }
            }
            ctx.currentBattle.reset();
            switchMode(GameState::overworld);

            MusicTrack mapTrack = MusicManager::trackForMap(world.getCurrentMapId());
            music.play(mapTrack, ui.getRenderer().getWindow());
            break;
        }

        case ModeRequest::Type::transitionToMap: {
            const std::string &mapId = world.getCurrentMapId();
            world.getMap(mapId).setOccupied(world.getPlayer().getPosition(), false);
            switchToMode(GameState::transition, std::make_unique<TransitionMode>(req.mapId, req.spawn));
            break;
        }

        case ModeRequest::Type::startDialogue: {
            dialogueReturnState = req.returnState;
            switchToMode(GameState::dialogue,
                         std::make_unique<DialogueMode>(req.speaker, req.lines, req.npc, req.returnState));
            break;
        }

        case ModeRequest::Type::startDialogueChoice: {
            switchToMode(GameState::dialogueChoice, std::make_unique<DialogueChoiceMode>(
                                                        req.choiceOptions, req.choiceContext, dialogueReturnState));
            break;
        }

        case ModeRequest::Type::startCutscene: {
            if (cutsceneRunner.load(req.cutscenePath)) {
                cutsceneRunner.start();
                switchMode(GameState::cutscene);
            }
            break;
        }

        case ModeRequest::Type::handleStoryAction:
            handleStoryAction(req.storyActionData);
            break;
        }
    }
}

// ── Story action dispatch
// ──────────────────────────────────────────────────────

void Session::handleStoryAction(const StoryAction &action) {
    switch (action.type) {
    case StoryAction::Type::startBattle:
        ctx.pushRequest(ModeRequest::trainerBattle(action.trainer));
        break;

    case StoryAction::Type::showChoice:
        ctx.pushRequest(ModeRequest::dialogueChoice(action.options, action.choiceContext, dialogueReturnState));
        break;

    case StoryAction::Type::showDialogue:
        ctx.pushRequest(ModeRequest::dialogue(action.speaker, action.lines, nullptr, GameState::overworld));
        break;

    case StoryAction::Type::startCutscene:
        ctx.pushRequest(ModeRequest::cutscene(action.cutscenePath));
        break;

    case StoryAction::Type::returnToState:
    default:
        if (ctx.pendingPushBack) {
            ctx.pendingPushBack = false;
            world.pushPlayerBack();
        }
        switchMode(dialogueReturnState);
        break;
    }
}

// ── Mode switching
// ─────────────────────────────────────────────────────────────

void Session::switchMode(GameState newState) { switchToMode(newState, createMode(newState)); }

void Session::switchToMode(GameState newState, std::unique_ptr<GameMode> mode) {
    if (currentMode)
        currentMode->onExit(ctx);

    currentMode = std::move(mode);
    currentState = newState;
    ui.setScreen(screenForState(newState));

    if (currentMode)
        currentMode->onEnter(ctx);
}

std::unique_ptr<GameMode> Session::createMode(GameState state) {
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

// ── GameState → ScreenType
// ─────────────────────────────────────────────────────

ScreenType Session::screenForState(GameState gs) {
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

// ── Getters
// ────────────────────────────────────────────────────────────────────

World &Session::getWorld() { return world; }
Pokedex &Session::getPokedex() { return pokedex; }
GameUI &Session::getUI() { return ui; }
SaveManager &Session::getSaveManager() { return saveManager; }
