#pragma once
#include "../data/Pokedex.h"
#include "../state/World.h"
#include <string>
#include <vector>

// Result of a story check — tells Session what to do next
struct StoryAction {
    enum class Type {
        none,          // No special action
        blockWarp,     // Prevent the warp and show dialogue
        showChoice,    // Show a choice box after dialogue ends
        startBattle,   // Start a trainer battle
        showDialogue,  // Show new dialogue lines
        returnToState, // Return to the dialogue-return state
        startCutscene, // Start a cutscene from file
    };

    Type type{Type::none};

    // For blockWarp / showDialogue
    std::string speaker;
    std::vector<std::string> lines;

    // For showChoice
    std::vector<std::string> options;
    std::string choiceContext;

    // For startBattle
    std::shared_ptr<NPC> trainer;

    // For startCutscene
    std::string cutscenePath;
};

class StoryManager {
  public:
    StoryManager() = default;

    // Called when a dialogue finishes — decides what happens next
    // npc: the NPC whose dialogue just ended (may be nullptr)
    StoryAction onDialogueEnd(std::shared_ptr<NPC> npc, World &world);

    // Called when the player picks a choice — decides what happens next
    StoryAction onChoiceSelected(const std::string &context, int choice, World &world,
                                 Pokedex &pokedex);

    // Called when the player steps on a warp — returns blockWarp action if
    // blocked
    StoryAction checkWarp(const WarpPoint &warp, Player &player);

    // Called when the player enters a new map — may trigger a cutscene
    StoryAction checkMapEnter(const std::string &mapId, Player &player);
};
