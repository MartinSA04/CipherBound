#pragma once
#include "../../app/GameMode.h"
#include "../ui/BattleRenderer.h"

class BattleMode : public GameMode {
  public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

  private:
    void updateBattleIntroAnim(GameContext &ctx);
    void updateCaptureAnim(GameContext &ctx);

    int menuSelected{0};
    int moveSelected{0};
    int partySelected{0};
    int bagSelected{0};

    // Battle party sub-state (for viewing summary or action menu from party
    // list)
    bool viewingSummary{false};
    bool showingPartyAction{false};
    int partyActionSelected{0};
    int summaryPage{0};

    // Capture animation state
    int captureAnimFrame{0};
    bool captureAnimDone{false};

    // Attack animation state
    int attackAnimFrame{0};

    // Idle bob animation counter (increments every frame in battle)
    int battleAnimFrame{0};

    // EXP animation sound state
    bool expSoundPlayed{false};

    BattleRenderer battleRenderer;
};
