/**
 * @file
 * @brief Shop mode for buying items from NPC inventories.
 * @ingroup app_core
 */

#pragma once

#include "../GameMode.h"
#include <string>
#include <vector>

/// Mode for browsing a shop inventory and confirming purchases.
class ShopMode : public GameMode {
  public:
    /// Creates a shop mode with title, owner, and sellable item ids.
    ShopMode(std::string title = "Shop", std::string shopkeeperName = {},
             std::vector<int> itemIds = {});

    /// Performs any setup required when entering the shop.
    void onEnter(GameContext &ctx) override;
    /// Updates browsing, quantity, and confirmation phases.
    void update(GameContext &ctx, InputManager &input) override;
    /// Renders the current shop phase.
    void render(GameContext &ctx) override;

  private:
    /// Current phase of the shop flow.
    enum class Phase {
        browsing,           ///< Browsing the shop inventory.
        choosingQuantity,   ///< Choosing the quantity to buy.
        confirmingPurchase, ///< Confirming the purchase.
    };

    void showMessage(GameContext &ctx, std::string text);
    int maxPurchaseQuantity(const Player &player, const ItemData &item) const;
    int currentPurchaseTotal(const Pokedex &pokedex) const;
    const ItemData &selectedItem(const Pokedex &pokedex) const;

    std::string shopTitle;        ///< Shop UI title.
    std::string shopkeeperName;   ///< Shopkeeper display name.
    std::vector<int> itemIds;     ///< Sellable item ids.
    Phase phase{Phase::browsing}; ///< Current shop phase.
    int selected{0};              ///< Current item selection.
    int purchaseQuantity{1};      ///< Quantity selected for purchase.
    int confirmSelected{0};       ///< Confirmation prompt selection.
    std::string message;          ///< Active status/purchase message.
    bool messageActive{false};    ///< Whether a status message is currently shown.
};
