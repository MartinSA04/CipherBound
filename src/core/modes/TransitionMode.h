#pragma once
#include "../GameMode.h"
#include <string>

class TransitionMode : public GameMode {
  public:
    TransitionMode(const std::string &targetMapId, const Position &targetSpawn);

    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

  private:
    std::string targetMapId;
    Position targetSpawn;
    bool fadeOut{true};
};
