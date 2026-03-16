#pragma once
#include "../GameMode.h"
#include <string>
#include <vector>

class NPC;

class DialogueMode : public GameMode {
  public:
    DialogueMode(const std::string &speaker, const std::vector<std::string> &lines, NPC *npc,
                 GameState returnState);

    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;
    void onEnter(GameContext &ctx) override;

  private:
    NPC *dialogueNPC;
    GameState returnState;
    std::string savedSpeaker;
    std::vector<std::string> savedLines;
};
