/**
 * @file
 * @brief Game-data catalog for species, moves, and items loaded from text files.
 * @ingroup app_core
 */

#pragma once
#include "Item.h"
#include "Move.h"
#include "Species.h"
#include <vector>

/**
 * @brief In-memory catalog of species, moves, and items.
 * @ingroup app_core
 *
 * Each table keeps index 0 as an unused dummy entry so game data ids can be
 * used directly as vector indexes.
 */
class Pokedex {
  public:
    /// Constructs empty tables with dummy index-0 entries.
    Pokedex();

    /// Loads species data from a pipe-delimited text file.
    void loadSpecies(const std::string &path);
    /// Loads move data from a pipe-delimited text file.
    void loadMoves(const std::string &path);
    /// Loads item data from a pipe-delimited text file.
    void loadItems(const std::string &path);

    /// Returns the species record for a positive id.
    const Species &getSpecies(int id) const;
    /// Returns the move record for a positive id.
    const MoveData &getMove(int id) const;
    /// Returns the item record for a positive id.
    const ItemData &getItem(int id) const;

    /// Returns the current species table size.
    int speciesCount() const;
    /// Returns the current move table size.
    int moveCount() const;
    /// Returns the current item table size.
    int itemCount() const;

  private:
    std::vector<Species> species;
    std::vector<MoveData> moves;
    std::vector<ItemData> items;
};
