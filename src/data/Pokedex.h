#pragma once
#include "Item.h"
#include "Move.h"
#include "Species.h"
#include <vector>

class Pokedex {
  public:
    Pokedex();

    void loadSpecies(const std::string &path);
    void loadMoves(const std::string &path);
    void loadItems(const std::string &path);

    const Species &getSpecies(int id) const;
    const MoveData &getMove(int id) const;
    const ItemData &getItem(int id) const;

    int speciesCount() const;
    int moveCount() const;
    int itemCount() const;

  private:
    std::vector<Species> species;
    std::vector<MoveData> moves;
    std::vector<ItemData> items;
};
