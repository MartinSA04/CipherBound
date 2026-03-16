#pragma once
#include "../GameMode.h"
#include <string>

class PCBoxMode : public GameMode {
  public:
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;
    void onEnter(GameContext &ctx) override;

  private:
    int selected{0};
    bool viewingParty{true};
    bool showingMessage{false};
    std::string message;
    bool lrHeld{false};
};
