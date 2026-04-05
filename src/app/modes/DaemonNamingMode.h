/**
 * @file
 * @brief Naming prompt shown for newly obtained daemons.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"
#include "NameEntryPanel.h"

/// Mode that lets the player edit a nickname for a newly obtained daemon.
class DaemonNamingMode : public GameMode {
  public:
    DaemonNamingMode(Daemon daemon, DaemonNamingPurpose purpose, std::string completionSpeaker = {},
                     std::vector<std::string> completionLines = {},
                     GameState returnState = GameState::overworld);

    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

  private:
    void finishNaming(GameContext &ctx);

    Daemon daemon;
    DaemonNamingPurpose purpose;
    std::string completionSpeaker;
    std::vector<std::string> completionLines;
    GameState returnState{GameState::overworld};

    NameEntryPanel nameEntry{"DEFAULT"};
};
