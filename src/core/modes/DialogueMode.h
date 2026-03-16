#pragma once
#include "../GameMode.h"
#include <memory>
#include <string>
#include <vector>

class NPC;

class DialogueMode : public GameMode {
  public:
    DialogueMode(const std::string &speaker, const std::vector<std::string> &lines,
                 std::shared_ptr<NPC> npc, GameState returnState);

    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;
    void onEnter(GameContext &ctx) override;

  private:
    std::shared_ptr<NPC> dialogueNPC;
    GameState returnState;
    std::string savedSpeaker;
    std::vector<std::string> savedLines;
};
