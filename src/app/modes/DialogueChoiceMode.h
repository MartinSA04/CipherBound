/**
 * @file
 * @brief Dialogue choice mode.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"
#include <string>
#include <vector>

/// Mode that lets the player choose between dialogue/story options.
class DialogueChoiceMode : public GameMode {
  public:
    /// Stores choice options and the story context used to interpret them.
    DialogueChoiceMode(const std::vector<std::string> &options, const std::string &context,
                       GameState returnState);

    /// Updates choice navigation and dispatches the selected result.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the active dialogue choice box.
    void render(GameContext &ctx) override;

  private:
    std::vector<std::string> choiceOptions; ///< Options shown to the player.
    int choiceSelected{0};                  ///< Current selection index.
    std::string choiceContext;              ///< Story context key for resolution.
    GameState returnState;                  ///< State resumed after the choice resolves.
};
