#pragma once
#include "../GameMode.h"

class MenuMode : public GameMode {
  public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

  private:
    int selected{0};
};
