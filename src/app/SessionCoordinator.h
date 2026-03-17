#pragma once

#include "GameMode.h"
#include <memory>

class InputManager;
enum class ScreenType;

class SessionCoordinator {
  public:
    explicit SessionCoordinator(GameContext &ctx);

    void update(InputManager &input);
    void render();
    void processRequests();
    void switchMode(GameState newState);

  private:
    void handleRequest(const ChangeStateRequest &req);
    void handleRequest(const EnterBattleModeRequest &req);
    void handleRequest(const StartWildBattleRequest &req);
    void handleRequest(const StartTrainerBattleRequest &req);
    void handleRequest(const StartTrainerBattleIntroRequest &req);
    void handleRequest(const EndBattleRequest &req);
    void handleRequest(const TransitionToMapRequest &req);
    void handleRequest(const StartDialogueRequest &req);
    void handleRequest(const StartDialogueChoiceRequest &req);
    void handleRequest(const StartCutsceneRequest &req);
    void handleRequest(const StoryActionRequest &req);
    void handleStoryAction(const StoryNoAction &action);
    void handleStoryAction(const StoryBlockWarpAction &action);
    void handleStoryAction(const StoryShowChoiceAction &action);
    void handleStoryAction(const StoryStartBattleAction &action);
    void handleStoryAction(const StoryShowDialogueAction &action);
    void handleStoryAction(const StoryReturnToStateAction &action);
    void handleStoryAction(const StoryStartCutsceneAction &action);

    void switchToMode(GameState newState, std::unique_ptr<GameMode> mode);
    std::unique_ptr<GameMode> createMode(GameState state);
    static ScreenType screenForState(GameState gs);

    GameContext &ctx;
    std::unique_ptr<GameMode> currentMode;
    GameState currentState{GameState::titleScreen};
};
