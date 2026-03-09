#pragma once
#include <vector>
#include <optional>
#include "Species.h"
#include "Move.h"
#include "Item.h"

class Pokedex
{
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
