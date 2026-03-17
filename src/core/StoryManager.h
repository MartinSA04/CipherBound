#pragma once
#include "StoryAction.h"
#include <string>

class NPC;
class Pokedex;
class Player;
class World;
struct WarpPoint;

class StoryManager {
  public:
    StoryManager() = default;

    // Called when a dialogue finishes — decides what happens next
    // npc: the NPC whose dialogue just ended (may be nullptr)
    StoryAction onDialogueEnd(NPC *npc, World &world);

    // Called when the player picks a choice — decides what happens next
    StoryAction onChoiceSelected(const std::string &context, int choice, World &world,
                                 Pokedex &pokedex);

    // Called when the player steps on a warp — returns blockWarp action if
    // blocked
    StoryAction checkWarp(const WarpPoint &warp, Player &player);

    // Called when the player enters a new map — may trigger a cutscene
    StoryAction checkMapEnter(const std::string &mapId, Player &player);
};
