#pragma once
#include "../GameMode.h"
#include "../../battle/ui/BattleRenderer.h"
#include <string>

class TransitionMode : public GameMode {
  public:
    enum class Kind {
        mapWarp,
        blackoutRespawn,
    };

    TransitionMode(const std::string &targetMapId, const Position &targetSpawn);
    explicit TransitionMode(Kind kind);

    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;
    void onEnter(GameContext &ctx) override;

  private:
    enum class Phase {
        fadingOut,
        fadingIn,
    };

    void completeFadeOut(GameContext &ctx);

    Kind kind{Kind::mapWarp};
    Phase phase{Phase::fadingOut};
    std::string targetMapId;
    Position targetSpawn;
    int phaseFrame{0};
    int phaseDuration{1};
    bool renderBattleBackdrop{false};
    BattleRenderer battleRenderer;
};
