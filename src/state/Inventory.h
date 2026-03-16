#pragma once
#include <vector>

struct InventoryEntry {
    int itemId;
    int quantity;
};

class Inventory {
  public:
    void addItem(int itemId, int quantity = 1);
    bool removeItem(int itemId, int quantity = 1);
    int getItemCount(int itemId) const;
    const std::vector<InventoryEntry> &getInventory() const;
    void clearInventory();

    // Currency
    int getMoney() const;
    void addMoney(int amount);
    bool spendMoney(int amount);
    void setMoney(int amount);

  private:
    std::vector<InventoryEntry> inventory;
    int money{0};
};
