#pragma once

#include "../GameMode.h"
#include <string>
#include <vector>

class ShopMode : public GameMode {
  public:
    ShopMode(std::string title = "Shop", std::string shopkeeperName = {},
             std::vector<int> itemIds = {});

    void onEnter(GameContext &ctx) override;
    void update(GameContext &ctx, InputManager &input) override;
    void render(GameContext &ctx) override;

  private:
    enum class Phase {
        browsing,
        choosingQuantity,
        confirmingPurchase,
    };

    void showMessage(GameContext &ctx, std::string text);
    int maxPurchaseQuantity(const Player &player, const ItemData &item) const;
    int currentPurchaseTotal(const Pokedex &pokedex) const;
    const ItemData &selectedItem(const Pokedex &pokedex) const;

    std::string shopTitle;
    std::string shopkeeperName;
    std::vector<int> itemIds;
    Phase phase{Phase::browsing};
    int selected{0};
    int purchaseQuantity{1};
    int confirmSelected{0};
    std::string message;
    bool messageActive{false};
};
