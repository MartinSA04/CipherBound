#pragma once
#include "../../app/GameMode.h"
class NPC;

class BattleIntroMode : public GameMode {
  public:
    // Wild battle intro
    BattleIntroMode(int speciesId, int level);
    // Trainer battle intro
    BattleIntroMode(NPC *trainer);

    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

  private:
    int speciesId{0};
    int level{0};
    NPC *trainer{nullptr};
};
