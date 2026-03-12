#pragma once
#include "../GameMode.h"
#include <string>
#include <memory>

class Battle;
class NPC;
class Renderer;

class BattleMode : public GameMode
{
public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

    void setTrainerNPCId(const std::string &id);
    void setTrainer(std::shared_ptr<NPC> trainer);
    const std::string &getTrainerNPCId() const;

private:
    void updateBattleIntroAnim(GameContext &ctx);
    void updateCaptureAnim(GameContext &ctx);

    // Battle rendering helpers (moved from GameUI)
    void drawBattleScene(GameContext &ctx);
    void drawBattleIntroSceneWild(GameContext &ctx);
    void drawBattleIntroSceneTrainer(GameContext &ctx);
    void drawBattleMenu(GameContext &ctx);
    void drawMoveSelectScreen(GameContext &ctx);
    void drawCaptureScene(GameContext &ctx);
    void drawBall(Renderer &renderer, int frame, int x, int y) const;
    void drawBallCentered(Renderer &renderer, int frame, int cx, int cy) const;

    std::string currentTrainerNPCId;
    std::shared_ptr<NPC> battleTrainer;

    int menuSelected{0};
    int moveSelected{0};
    int partySelected{0};
    int bagSelected{0};

    // Battle party sub-state (for viewing summary or action menu from party list)
    bool viewingSummary{false};
    bool showingPartyAction{false};
    int partyActionSelected{0};
    int summaryPage{0};

    // Capture animation state
    int captureAnimFrame{0};
    int captureAnimShakesDone{0};
    bool captureAnimDone{false};

    // Attack animation state
    int attackAnimFrame{0};

    // Idle bob animation counter (increments every frame in battle)
    int battleAnimFrame{0};

    // EXP animation sound state
    bool expSoundPlayed{false};
};
