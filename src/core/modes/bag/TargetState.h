#pragma once
#include "BagSubState.h"

class TargetState : public BagSubState {
  public:
    void update(BagMode &bag, GameContext &ctx, InputManager &input) override;
    void render(BagMode &bag, GameContext &ctx) override;
};
