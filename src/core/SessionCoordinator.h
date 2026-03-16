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
    void handleChangeStateRequest(const ModeRequest &req);
    void handleStartWildBattleRequest(const ModeRequest &req);
    void handleStartTrainerBattleRequest(const ModeRequest &req);
    void handleEndBattleRequest();
    void handleTransitionToMapRequest(const ModeRequest &req);
    void handleStartDialogueRequest(const ModeRequest &req);
    void handleStartDialogueChoiceRequest(const ModeRequest &req);
    void handleStartCutsceneRequest(const ModeRequest &req);
    void handleStoryAction(const StoryAction &action);

    void switchToMode(GameState newState, std::unique_ptr<GameMode> mode);
    std::unique_ptr<GameMode> createMode(GameState state);
    static ScreenType screenForState(GameState gs);

    GameContext &ctx;
    std::unique_ptr<GameMode> currentMode;
    GameState currentState{GameState::titleScreen};
};
