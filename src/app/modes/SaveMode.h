#pragma once
#include "../GameMode.h"

class SaveMode : public GameMode {
  public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;
    void onEnter(GameContext &ctx) override;

  private:
    bool saveComplete{false};
    bool saveSuccess{false};
};
