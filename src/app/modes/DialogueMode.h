/**
 * @file
 * @brief Linear dialogue mode.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"
#include <string>
#include <vector>

class NPC;

/// Mode that displays one speaker and a sequence of dialogue lines.
class DialogueMode : public GameMode {
  public:
    /// Stores dialogue payload for playback when the mode becomes active.
    DialogueMode(const std::string &speaker, const std::vector<std::string> &lines, NPC *npc,
                 GameState returnState);

    /// Updates typewriter state and dialogue advancement.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the overworld backdrop and dialogue box.
    void render(GameContext &ctx) override;
    /// Starts the stored dialogue sequence in the shared UI.
    void onEnter(GameContext &ctx) override;

  private:
    NPC *dialogueNPC;                    ///< Optional NPC tied to the dialogue outcome.
    GameState returnState;               ///< State resumed after dialogue finishes.
    std::string savedSpeaker;            ///< Stored speaker name.
    std::vector<std::string> savedLines; ///< Stored dialogue lines.
};
