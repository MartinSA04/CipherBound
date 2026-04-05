/**
 * @file
 * @brief Title screen and save-slot selection mode.
 * @ingroup app_core
 */

#pragma once
#include "../../save/SaveManager.h"
#include "../GameMode.h"
#include "NameEntryPanel.h"
#include <vector>

/// Opening mode showing the title card and available save slots.
class TitleScreenMode : public GameMode {
  public:
    /// Updates title-card and save-slot selection flow.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the title card or save-slot picker.
    void render(GameContext &ctx) override;
    /// Refreshes slot summaries when the mode becomes active.
    void onEnter(GameContext &ctx) override;

  private:
    /// Phase of the title-screen flow.
    enum class Phase {
        titleCard,      ///< Show the title and wait for confirmation.
        saveSlotSelect, ///< Show save slots and allow selection.
        nameEntry,      ///< Enter a new player name for an empty save slot.
    };

    Phase phase{Phase::titleCard};    ///< Active title-screen phase.
    int selected{0};                  ///< Selected save slot index.
    int titleTimer{0};                ///< Title-card timer.
    NameEntryPanel nameEntry{"BACK"}; ///< Shared name-entry panel for player naming.

    std::vector<SaveManager::SlotInfo> slotInfos; ///< Cached slot metadata.
};
