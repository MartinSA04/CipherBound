#pragma once
#include "BagSubState.h"

class MessageState : public BagSubState {
  public:
    void update(BagMode &bag, GameContext &ctx, InputManager &input) override;
    void render(BagMode &bag, GameContext &ctx) override;
};
