#pragma once
#include <string>
#include <vector>

struct TextBox {
    int x, y;
    int width, height;
    std::string text;
    bool isVisible;
};

struct MenuItem {
    std::string label;
    bool enabled;
};

class Menu {
  public:
    Menu();

    void addItem(const std::string &label, bool enabled = true);
    void setItems(const std::vector<MenuItem> &items);
    void clear();

    int getSelectedIndex() const;
    void moveUp();
    void moveDown();
    void select();

    const std::vector<MenuItem> &getItems() const;
    bool isVisible() const;
    void setVisible(bool visible);

  private:
    std::vector<MenuItem> items;
    int selectedIndex;
    bool visible;
};
