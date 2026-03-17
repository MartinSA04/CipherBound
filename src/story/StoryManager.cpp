#include "StoryManager.h"
#include "../game_data/Pokedex.h"
#include "../state/World.h"

StoryAction StoryManager::onDialogueEnd(NPC *npc, World &world) {
    // Check for trainer battle
    if (npc && !npc->isDefeated() &&
        (npc->getType() == NPCType::trainer || npc->getType() == NPCType::gymLeader) &&
        !npc->partyEmpty()) {
        return StoryAction::startBattle(npc);
    }

    // Pokeball on table — offer yes/no choice for that starter
    if (npc && npc->getId().starts_with("pokeball_") && !world.getPlayer().hasFlag("has_starter")) {
        return StoryAction::showChoice({"Yes", "No"}, npc->getId());
    }

    // Default: return to previous state
    return StoryAction::returnToState();
}

StoryAction StoryManager::onChoiceSelected(const std::string &context, int choice, World &world,
                                           Pokedex &pokedex) {
    // Pokeball yes/no choice: choice 0 = Yes, 1 = No
    if (context.starts_with("pokeball_") && choice == 0) {
        // Extract species id from "pokeball_N"
        int speciesId = std::stoi(context.substr(9));
        const Species &species = pokedex.getSpecies(speciesId);
        Daemon starter{species, 5};
        world.getPlayer().addDaemon(starter);
        world.getPlayer().setFlag("has_starter");

        // Show confirmation dialogue
        return StoryAction::showDialogue(
            "Prof. Bart Iver",
            {"So you chose " + species.name + "!", "Take good care of it.",
             "The world is wide! Go explore!"});
    }

    // Default: return to previous state
    return StoryAction::returnToState();
}

StoryAction StoryManager::checkWarp(const WarpPoint &warp, Player &player) {
    // Block route_1 if player has no starter
    if (warp.targetMapId == "route_1" && !player.hasFlag("has_starter")) {
        return StoryAction::blockWarp({"Prof. Bart Iver's words echoed...",
                                       "You can't go out without a Daemon!",
                                       "Go see Prof. Bart Iver in his lab."});
    }

    return StoryAction::none();
}

StoryAction StoryManager::checkMapEnter(const std::string &mapId, Player &player) {
    // Trigger Bart Iver intro cutscene on first visit
    if (mapId == "bart_iver_lab" && !player.hasFlag("bart_iver_intro_done")) {
        return StoryAction::startCutscene("assets/data/cutscenes/bart_iver_intro.cutscene");
    }

    return StoryAction::none();
}
