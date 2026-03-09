#include "StoryManager.h"

StoryAction StoryManager::onDialogueEnd(std::shared_ptr<NPC> npc, World &world, Pokedex &pokedex)
{
    // Check for trainer battle
    if (npc && !npc->isDefeated() &&
        (npc->getType() == NPCType::trainer || npc->getType() == NPCType::gymLeader) &&
        !npc->getParty().empty())
    {
        StoryAction action;
        action.type = StoryAction::Type::startBattle;
        action.trainer = npc;
        return action;
    }

    // Pokeball on table — offer yes/no choice for that starter
    if (npc && npc->getId().starts_with("pokeball_") && !world.getPlayer().hasFlag("has_starter"))
    {
        StoryAction action;
        action.type = StoryAction::Type::showChoice;
        action.options = {"Yes", "No"};
        action.choiceContext = npc->getId(); // e.g. "pokeball_1"
        return action;
    }

    // Default: return to previous state
    StoryAction action;
    action.type = StoryAction::Type::returnToState;
    return action;
}

StoryAction StoryManager::onChoiceSelected(const std::string &context, int choice,
                                           World &world, Pokedex &pokedex)
{
    // Pokeball yes/no choice: choice 0 = Yes, 1 = No
    if (context.starts_with("pokeball_") && choice == 0)
    {
        // Extract species id from "pokeball_N"
        int speciesId = std::stoi(context.substr(9));
        const Species &species = pokedex.getSpecies(speciesId);
        Creature starter{species, 5};
        world.getPlayer().addCreature(starter);
        world.getPlayer().setFlag("has_starter");

        // Show confirmation dialogue
        StoryAction action;
        action.type = StoryAction::Type::showDialogue;
        action.speaker = "Prof. Oak";
        action.lines = {
            "So you chose " + species.name + "!",
            "Take good care of it.",
            "The world is wide! Go explore!"};
        return action;
    }

    // Default: return to previous state
    StoryAction action;
    action.type = StoryAction::Type::returnToState;
    return action;
}

StoryAction StoryManager::checkWarp(const WarpPoint &warp, Player &player)
{
    // Block route_1 if player has no starter
    if (warp.targetMapId == "route_1" && !player.hasFlag("has_starter"))
    {
        StoryAction action;
        action.type = StoryAction::Type::blockWarp;
        action.lines = {
            "Prof. Oak's words echoed...",
            "You can't go out without a creature!",
            "Go see Prof. Oak in his lab."};
        return action;
    }

    StoryAction action;
    action.type = StoryAction::Type::none;
    return action;
}

StoryAction StoryManager::checkMapEnter(const std::string &mapId, Player &player)
{
    // Trigger Oak intro cutscene on first visit
    if (mapId == "oak_lab" && !player.hasFlag("oak_intro_done"))
    {
        StoryAction action;
        action.type = StoryAction::Type::startCutscene;
        action.cutscenePath = "assets/data/cutscenes/oak_intro.cutscene";
        return action;
    }

    StoryAction action;
    action.type = StoryAction::Type::none;
    return action;
}
