/**
 * @file
 * @brief Bag/inventory mode with dedicated sub-states.
 * @ingroup app_core
 */

#pragma once
#include "../GameMode.h"
#include "bag/BagSubState.h"
#include <memory>
#include <string>

/// Inventory mode that delegates detailed behavior to bag substate objects.
class BagMode : public GameMode {
  public:
    /// High-level bag substates.
    enum class SubStateType {
        browsing,       ///< Browsing items.
        choosingTarget, ///< Choosing a party target for an item.
        showingMessage, ///< Showing a transient bag message.
    };

    /// Creates the bag mode in its default browsing state.
    BagMode();

    /// Updates the active bag substate.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the active bag substate.
    void render(GameContext &ctx) override;

    /// Switches the active bag substate implementation.
    void switchSubState(SubStateType type);
    /// Switches to the message substate with the given text.
    void showMessage(const std::string &msg, GameContext &ctx);

    int selected{0};               ///< Current bag selection.
    int partySelected{0};          ///< Current party target selection.
    int useItemId{0};              ///< Item being used in the current flow.
    std::string message;           ///< Active bag message text.

    SubStateType returnAfterMessage{SubStateType::browsing}; ///< Substate restored after a message.

  private:
    std::unique_ptr<BagSubState> createSubState(SubStateType type);
    std::unique_ptr<BagSubState> currentSubState; ///< Owned active substate object.
};
