#include "StoryManager.h"
#include "../game_data/Pokedex.h"
#include "../state/World.h"

std::optional<int> StoryManager::starterSpeciesIdForChoiceContext(const std::string &context) {
    if (context == "pokeball_1")
        return 1;
    if (context == "pokeball_2")
        return 4;
    if (context == "pokeball_3")
        return 7;
    return std::nullopt;
}

StoryAction StoryManager::onDialogueEnd(NPC *npc, World &world) {
    // Check for trainer battle
    if (!npc)
        return StoryAction::returnToState();

    if (!npc->isDefeated() &&
        (npc->getType() == NPCType::trainer || npc->getType() == NPCType::gymLeader) &&
        !npc->partyEmpty()) {
        return StoryAction::startBattle(npc);
    }

    // Pokeball on table — offer yes/no choice for that starter
    if (starterSpeciesIdForChoiceContext(npc->getId()).has_value() &&
        !world.getPlayer().hasFlag("has_starter")) {
        return StoryAction::showChoice({"Yes", "No"}, npc->getId());
    }

    // Intro mailbox
    if (npc->getId() == "pallet_mailbox")
        world.getPlayer().setFlag("read_mail");

    // Default: return to previous state
    return StoryAction::returnToState();
}

StoryAction StoryManager::onChoiceSelected(const std::string &context, int choice, World &world,
                                           Pokedex &pokedex) {
    // Pokeball yes/no choice: choice 0 = Yes, 1 = No
    if (const auto speciesId = starterSpeciesIdForChoiceContext(context);
        speciesId.has_value() && choice == 0) {
        const Species &species = pokedex.getSpecies(*speciesId);
        Daemon starter = Daemon::generateRandomized(species, 5, world.getRng());
        world.getPlayer().addDaemon(starter);
        world.getPlayer().setFlag("has_starter");

        // Show confirmation dialogue
        return StoryAction::showDialogue(
            "Prof. Bart Iver", {"So you chose " + species.name + "!", "Take good care of it.",
                                "Go to the first faculty and retrieve what's mine.",
                                "I promise you that everything will be explained."});
    }

    // Default: return to previous state
    return StoryAction::returnToState();
}

StoryAction StoryManager::checkWarp(const WarpPoint &warp, Player &player) {

    if (warp.targetMapId == "route_1") {
        if (!player.hasFlag("read_mail")) {
            return StoryAction::blockWarp({"I need to get my schedule in the mail first.",
                                           "I won't know where to go without it."});
        }

        if (!player.hasFlag("bart_iver_intro_done")) {
            return StoryAction::blockWarp(
                {"I should go see what that guy wants first.", "It might be important."});
        }

        if (!player.hasFlag("has_starter")) {
            return StoryAction::blockWarp({"I need to pick my Deamon first."});
        }
    }

    if (warp.targetMapId == "bart_iver_lab") {
        if (!player.hasFlag("read_mail")) {
            return StoryAction::blockWarp({"I shouldn't just go into someones lab."});
        }
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
