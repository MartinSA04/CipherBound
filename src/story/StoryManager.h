/**
 * @file
 * @brief Story progression hooks invoked by dialogue, choices, warps, and map entry.
 * @ingroup app_core
 */

#pragma once
#include "StoryAction.h"
#include <optional>
#include <string>
#include <vector>

class NPC;
class Pokedex;
class Player;
class World;
struct WarpPoint;

/**
 * @brief Central story-rule dispatcher used by the app flow.
 * @ingroup app_core
 */
class StoryManager {
  public:
    struct ObjectiveInfo {
        std::string title;
        std::vector<std::string> lines;
    };

    StoryManager() = default;

    /// Returns the species id associated with a starter-choice context, if any.
    static std::optional<int> starterSpeciesIdForChoiceContext(const std::string &context);

    /// Decides what should happen after dialogue closes.
    StoryAction onDialogueEnd(NPC *npc, World &world);

    /// Decides what should happen after the player makes a dialogue choice.
    StoryAction onChoiceSelected(const std::string &context, int choice, World &world,
                                 Pokedex &pokedex);

    /// Returns a warp-blocking story action when entering the warp is disallowed.
    StoryAction checkWarp(const WarpPoint &warp, Player &player);

    /// Returns any story action triggered by entering a new map.
    StoryAction checkMapEnter(const std::string &mapId, Player &player);

    /// Returns the current main objective shown in the pause menu.
    ObjectiveInfo currentObjective(const Player &player) const;
};
