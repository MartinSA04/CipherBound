#pragma once
#include "../GameMode.h"
#include "../../save/SaveManager.h"
#include <vector>

class TitleScreenMode : public GameMode
{
public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;
    void onEnter(GameContext &ctx) override;

private:
    int selected{0};
    std::vector<SaveManager::SlotInfo> slotInfos;
};
