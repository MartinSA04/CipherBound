#pragma once
#include <string>
#include <vector>
#include "../world/Map.h" // Position, Direction

// A single step in a cutscene script
struct CutsceneStep
{
    enum class Type
    {
        move,  // Move entity tile-by-tile to (x,y). Non-blocking (use sync).
        walk,  // Move entity one tile in direction. Non-blocking (use sync).
        face,  // Set entity facing direction. Immediate.
        say,   // Show dialogue. Blocking (waits for player dismiss).
        wait,  // Wait N frames. Blocking.
        sync,  // Wait for all pending movements to finish. Blocking.
        flag,  // Set a player event flag. Immediate.
        hide,  // Hide an NPC from the map. Immediate.
        show,  // Show a hidden NPC on the map. Immediate.
    };

    Type type;

    // Target entity: "player" or NPC id
    std::string target;

    // For move: destination
    int x{0}, y{0};

    // For walk / face: direction
    Direction direction{Direction::down};

    // For say: speaker + lines
    std::string speaker;
    std::vector<std::string> lines;

    // For wait: frame count
    int frames{0};

    // For flag: flag name
    std::string flagName;
};

// A complete cutscene loaded from file
struct Cutscene
{
    std::string id;
    std::vector<CutsceneStep> steps;
};
