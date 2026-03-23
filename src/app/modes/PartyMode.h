/**
 * @file
 * @brief Party management mode.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"

/// Mode for inspecting, rearranging, and reviewing party daemons.
class PartyMode : public GameMode {
  public:
    /// Updates party navigation, swapping, and summary flow.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the party UI or summary screen.
    void render(GameContext &ctx) override;

  private:
    /// Current party-mode substate.
    enum class SubState {
        browsing,       ///< Navigating the party list.
        actionMenu,     ///< Showing the action submenu.
        selectingSwap,  ///< Selecting a target to swap with.
        showingSummary, ///< Showing the daemon summary screen.
    };

    int selected{0};                       ///< Current party selection.
    SubState subState{SubState::browsing}; ///< Active party substate.
    int actionSelected{0};                 ///< Current action submenu selection.
    int swapSource{0};                     ///< Source party index for swaps.
    int summaryPage{0};                    ///< Active summary page.
    int summaryMoveSelected{0};            ///< Selected move on the summary move page.
};
