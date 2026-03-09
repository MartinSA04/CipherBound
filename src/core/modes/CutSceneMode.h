#pragma once
#include "../GameMode.h"

class CutSceneMode : public GameMode
{
public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;
};
