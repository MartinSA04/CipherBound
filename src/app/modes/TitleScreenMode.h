#pragma once
#include "../../save/SaveManager.h"
#include "../GameMode.h"
#include <vector>

class TitleScreenMode : public GameMode {
  public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;
    void onEnter(GameContext &ctx) override;

  private:
    enum class Phase {
        titleCard,      // Show game title, press any key
        saveSlotSelect, // Choose a save slot
    };

    Phase phase{Phase::titleCard};
    int selected{0};
    int titleTimer{0};

    std::vector<SaveManager::SlotInfo> slotInfos;
};
