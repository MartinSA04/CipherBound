#pragma once
#include "../GameMode.h"
#include <string>
#include <vector>

class DialogueChoiceMode : public GameMode
{
public:
    DialogueChoiceMode(const std::vector<std::string> &options,
                       const std::string &context,
                       GameState returnState);

    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

private:
    std::vector<std::string> choiceOptions;
    int choiceSelected{0};
    std::string choiceContext;
    GameState returnState;
};
