/**
 * @file
 * @brief In-memory cutscene data structures shared by parsing and playback code.
 * @ingroup cutscene_system
 * @ingroup data_formats
 */

#pragma once
#include "../state/Movement.h"
#include <string>
#include <vector>

/// A single command emitted by the cutscene parser.
struct CutsceneStep {
    /// Kind of command represented by the step.
    enum class Type {
        move, ///< Move an entity toward an absolute tile position.
        walk, ///< Move an entity one tile in a direction.
        face, ///< Change an entity's facing direction immediately.
        say,  ///< Show dialogue and block until dismissed.
        wait, ///< Wait a fixed number of frames.
        sync, ///< Wait for all queued moves to complete.
        flag, ///< Set a player event flag.
        badge, ///< Award a badge to the player.
        item, ///< Add an item directly to the player's inventory.
        hide, ///< Hide an NPC.
        show, ///< Show a previously hidden NPC.
    };

    Type type; ///< Command type.

    std::string target; ///< Target entity id, or `player`.

    int x{0}; ///< Destination X for `move`.
    int y{0}; ///< Destination Y for `move`.

    Direction direction{Direction::down}; ///< Direction for `walk` and `face`.

    std::string speaker;            ///< Speaker name for `say`.
    std::vector<std::string> lines; ///< Dialogue lines for `say`.

    int frames{0}; ///< Frame count for `wait`.

    std::string flagName; ///< Event flag name for `flag`.
    std::string badgeName; ///< Badge name for `badge`.
    int itemId{0};         ///< Item id for `item`.
    int itemQuantity{0};   ///< Quantity for `item`.
};

/// A complete parsed cutscene loaded from disk or created programmatically.
struct Cutscene {
    std::string id;                  ///< Stable cutscene id from the file header.
    std::vector<CutsceneStep> steps; ///< Ordered steps executed by the runner.
};
