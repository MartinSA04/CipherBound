#pragma once

class BagMode;
struct GameContext;
class InputManager;

class BagSubState {
  public:
    virtual ~BagSubState() = default;
    virtual void update(BagMode &bag, GameContext &ctx,
                        InputManager &input) = 0;
    virtual void render(BagMode &bag, GameContext &ctx) = 0;
};
