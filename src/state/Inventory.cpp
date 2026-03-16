#include "Inventory.h"

void Inventory::addItem(int itemId, int quantity) {
    for (auto &entry : inventory) {
        if (entry.itemId == itemId) {
            entry.quantity += quantity;
            return;
        }
    }
    inventory.push_back({itemId, quantity});
}

bool Inventory::removeItem(int itemId, int quantity) {
    for (auto it = inventory.begin(); it != inventory.end(); ++it) {
        if (it->itemId == itemId) {
            if (it->quantity < quantity)
                return false;
            it->quantity -= quantity;
            if (it->quantity <= 0)
                inventory.erase(it);
            return true;
        }
    }
    return false;
}

int Inventory::getItemCount(int itemId) const {
    for (const auto &entry : inventory) {
        if (entry.itemId == itemId)
            return entry.quantity;
    }
    return 0;
}

const std::vector<InventoryEntry> &Inventory::getInventory() const { return inventory; }

void Inventory::clearInventory() { inventory.clear(); }

// --- Money ---

int Inventory::getMoney() const { return money; }

void Inventory::addMoney(int amount) { money += amount; }

bool Inventory::spendMoney(int amount) {
    if (money < amount)
        return false;
    money -= amount;
    return true;
}

void Inventory::setMoney(int amount) { money = amount; }
