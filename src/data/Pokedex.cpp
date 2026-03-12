#include "Pokedex.h"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

Pokedex::Pokedex()
{
    // Index 0 is unused (dummy)
    species.push_back({});
    moves.push_back({});
    items.push_back({});
}

void Pokedex::loadSpecies(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open species file: " << path << std::endl;
        return;
    }

    // Clear existing species (keep dummy at index 0)
    species.resize(1);

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, '|'))
            tokens.push_back(token);

        if (tokens.size() < 14)
        {
            std::cerr << "Malformed species line (expected 14 fields): " << line << std::endl;
            continue;
        }

        Species sp;
        sp.id = std::stoi(tokens[0]);
        sp.name = tokens[1];

        auto pt = typeMap.find(tokens[2]);
        auto st = typeMap.find(tokens[3]);
        if (pt == typeMap.end() || st == typeMap.end())
        {
            std::cerr << "Unknown type for species " << sp.name << std::endl;
            continue;
        }
        sp.primaryType = pt->second;
        sp.secondaryType = st->second;

        sp.baseStats.hp = std::stoi(tokens[4]);
        sp.baseStats.attack = std::stoi(tokens[5]);
        sp.baseStats.defense = std::stoi(tokens[6]);
        sp.baseStats.specialAttack = std::stoi(tokens[7]);
        sp.baseStats.specialDefense = std::stoi(tokens[8]);
        sp.baseStats.speed = std::stoi(tokens[9]);
        sp.catchRate = std::stoi(tokens[10]);
        sp.baseExpYield = std::stoi(tokens[11]);

        // Parse learnset: "moveId:level,moveId:level,..."
        if (!tokens[12].empty() && tokens[12] != "none")
        {
            std::istringstream lss(tokens[12]);
            std::string entry;
            while (std::getline(lss, entry, ','))
            {
                auto colon = entry.find(':');
                if (colon != std::string::npos)
                {
                    int moveId = std::stoi(entry.substr(0, colon));
                    int level = std::stoi(entry.substr(colon + 1));
                    sp.learnset.push_back({moveId, level});
                }
            }
        }

        // Parse evolutions: "targetId:level,targetId:level,..."
        if (!tokens[13].empty() && tokens[13] != "none")
        {
            std::istringstream ess(tokens[13]);
            std::string entry;
            while (std::getline(ess, entry, ','))
            {
                auto colon = entry.find(':');
                if (colon != std::string::npos)
                {
                    int targetId = std::stoi(entry.substr(0, colon));
                    int level = std::stoi(entry.substr(colon + 1));
                    sp.evolutions.push_back({targetId, level});
                }
            }
        }

        // Insert at correct index
        while (static_cast<int>(species.size()) <= sp.id)
            species.push_back({});
        species[static_cast<size_t>(sp.id)] = sp;
    }
}

void Pokedex::loadMoves(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open moves file: " << path << std::endl;
        return;
    }

    // Clear existing moves (keep dummy at index 0)
    moves.resize(1);

    std::string line;
    while (std::getline(file, line))
    {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, '|'))
            tokens.push_back(token);

        if (tokens.size() < 10)
        {
            std::cerr << "Malformed move line (expected 10 fields): " << line << std::endl;
            continue;
        }

        MoveData move;
        move.id = std::stoi(tokens[0]);
        move.name = tokens[1];
        move.description = tokens[2];

        auto typeIt = typeMap.find(tokens[3]);
        if (typeIt == typeMap.end())
        {
            std::cerr << "Unknown type '" << tokens[3] << "' for move " << move.name << std::endl;
            continue;
        }
        move.type = typeIt->second;

        auto catIt = categoryMap.find(tokens[4]);
        if (catIt == categoryMap.end())
        {
            std::cerr << "Unknown category '" << tokens[4] << "' for move " << move.name << std::endl;
            continue;
        }
        move.category = catIt->second;

        move.power = std::stoi(tokens[5]);
        move.accuracy = std::stoi(tokens[6]);
        move.maxPP = std::stoi(tokens[7]);

        auto statusIt = statusMap.find(tokens[8]);
        if (statusIt == statusMap.end())
        {
            std::cerr << "Unknown status '" << tokens[8] << "' for move " << move.name << std::endl;
            continue;
        }
        move.statusEffect = statusIt->second;
        move.statusChance = std::stoi(tokens[9]);

        // Insert at correct index (pad with empty entries if needed)
        while (static_cast<int>(moves.size()) <= move.id)
            moves.push_back({});
        moves[static_cast<size_t>(move.id)] = move;
    }
}

void Pokedex::loadItems(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open items file: " << path << std::endl;
        return;
    }

    items.resize(1); // Keep dummy at index 0

    static const std::unordered_map<std::string, ItemCategory> catMap = {
        {"healing", ItemCategory::healing},
        {"capture", ItemCategory::capture},
        {"battle", ItemCategory::battle},
        {"keyItem", ItemCategory::keyItem},
    };

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, '|'))
            tokens.push_back(token);

        if (tokens.size() < 6)
        {
            std::cerr << "Malformed item line (expected 6 fields): " << line << std::endl;
            continue;
        }

        ItemData item;
        item.id = std::stoi(tokens[0]);
        item.name = tokens[1];
        item.description = tokens[2];

        auto catIt = catMap.find(tokens[3]);
        if (catIt == catMap.end())
        {
            std::cerr << "Unknown item category '" << tokens[3] << "' for item " << item.name << std::endl;
            continue;
        }
        item.category = catIt->second;
        item.value = std::stoi(tokens[4]);
        item.effectValue = std::stoi(tokens[5]);

        while (static_cast<int>(items.size()) <= item.id)
            items.push_back({});
        items[static_cast<size_t>(item.id)] = item;
    }
}

const Species &Pokedex::getSpecies(int id) const
{
    if (id < 0 || id >= static_cast<int>(species.size()))
        throw std::out_of_range("Species id out of range: " + std::to_string(id));
    return species[id];
}

const MoveData &Pokedex::getMove(int id) const
{
    if (id < 0 || id >= static_cast<int>(moves.size()))
        throw std::out_of_range("Move id out of range: " + std::to_string(id));
    return moves[id];
}

const ItemData &Pokedex::getItem(int id) const
{
    if (id < 0 || id >= static_cast<int>(items.size()))
        throw std::out_of_range("Item id out of range: " + std::to_string(id));
    return items[id];
}

int Pokedex::speciesCount() const { return static_cast<int>(species.size()); }
int Pokedex::moveCount() const { return static_cast<int>(moves.size()); }
int Pokedex::itemCount() const { return static_cast<int>(items.size()); }
