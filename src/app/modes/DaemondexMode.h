/**
 * @file
 * @brief Daemondex browsing mode.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"

/// Mode for browsing seen/caught daemon entries and detail pages.
class DaemondexMode : public GameMode {
  public:
    /// Updates list/detail navigation.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the list or detail view.
    void render(GameContext &ctx) override;

  private:
    /// Current daemondex view.
    enum class SubState {
        list,   ///< Scrollable list view.
        detail, ///< Detail page for one entry.
    };

    SubState subState{SubState::list}; ///< Active daemondex subview.
    int selected{0};                   ///< Selected entry index.
    int scrollOffset{0};               ///< List scroll offset.

    static constexpr int VISIBLE_ROWS = 8; ///< Number of rows visible in list view.

    void drawList(GameContext &ctx);
    void drawDetail(GameContext &ctx);
};
