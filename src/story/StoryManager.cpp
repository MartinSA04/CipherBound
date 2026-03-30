#include "StoryManager.h"
#include "../game_data/Pokedex.h"
#include "../state/World.h"

namespace {

constexpr const char *bartIntroCutscenePath = "assets/data/cutscenes/bart_iver_intro.cutscene";
constexpr const char *firstFacultyAftermathCutscenePath =
    "assets/data/cutscenes/first_faculty_aftermath.cutscene";
constexpr const char *rivalInterceptCutscenePath =
    "assets/data/cutscenes/rival_intercept.cutscene";
constexpr const char *bartConcordanceRevealCutscenePath =
    "assets/data/cutscenes/bart_concordance_reveal.cutscene";
constexpr const char *academyArchiveCutscenePath =
    "assets/data/cutscenes/academy_archive.cutscene";

} // namespace

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
        return StoryAction::promptStarterNickname(
            std::move(starter),
            "Prof. Bart Iver",
            {"Then " + species.name + " it is.",
             "Take care of it, and it will carry you farther than lectures ever could.",
             "Go to the first faculty and bring back anything sealed from my confiscated archive.",
             "When you return, I'll tell you why they were so desperate to bury it."});
    }

    // Default: return to previous state
    return StoryAction::returnToState();
}

StoryAction StoryManager::checkWarp(const WarpPoint &warp, Player &player) {

    if (warp.targetMapId == "route_1") {
        if (!player.hasFlag("read_mail")) {
            return StoryAction::blockWarp(
                {"I should check my mailbox first.", "If my schedule's there, I need it."});
        }

        if (!player.hasFlag("bart_iver_intro_done")) {
            return StoryAction::blockWarp(
                {"I should hear Bart out before I wander off.",
                 "If he sent that letter, it wasn't for nothing."});
        }

        if (!player.hasFlag("has_starter")) {
            return StoryAction::blockWarp({"I should choose a Daemon first."});
        }
    }

    if (warp.targetMapId == "bart_iver_lab") {
        if (!player.hasFlag("read_mail")) {
            return StoryAction::blockWarp(
                {"I shouldn't just walk into a stranger's lab for no reason."});
        }
    }

    return StoryAction::none();
}

StoryAction StoryManager::checkMapEnter(const std::string &mapId, Player &player) {
    if (mapId == "bart_iver_lab" && !player.hasFlag("bart_iver_intro_done")) {
        return StoryAction::startCutscene(bartIntroCutscenePath);
    }

    if (mapId == "viridian_faculty" && player.hasFlag("defeated_faculty_leader") &&
        !player.hasFlag("first_faculty_aftermath_done")) {
        return StoryAction::startCutscene(firstFacultyAftermathCutscenePath);
    }

    if (mapId == "viridian_town" && player.hasFlag("first_faculty_aftermath_done") &&
        !player.hasFlag("rival_intro_done")) {
        return StoryAction::startCutscene(rivalInterceptCutscenePath);
    }

    if (mapId == "bart_iver_lab" && player.hasFlag("received_concordance_index") &&
        player.hasFlag("rival_intro_done") && !player.hasFlag("bart_concordance_reveal_done")) {
        return StoryAction::startCutscene(bartConcordanceRevealCutscenePath);
    }

    if (mapId == "viridian_academy" && player.hasFlag("academy_archive_unlocked") &&
        !player.hasFlag("academy_archive_searched")) {
        return StoryAction::startCutscene(academyArchiveCutscenePath);
    }

    return StoryAction::none();
}

StoryManager::ObjectiveInfo StoryManager::currentObjective(const Player &player) const {
    if (!player.hasFlag("read_mail")) {
        return {"Check Your Mail",
                {"Read the letter waiting in your mailbox.",
                 "Bart Iver asked to meet you at the lab around the corner."}};
    }

    if (!player.hasFlag("bart_iver_intro_done")) {
        return {"Meet Bart Iver",
                {"Go to Bart Iver's lab in Pallet.",
                 "Hear why he called you there in secret."}};
    }

    if (!player.hasFlag("has_starter")) {
        return {"Choose A Daemon",
                {"Pick one of Bart Iver's hidden Daemons.",
                 "You will need it before you can challenge the first faculty."}};
    }

    if (!player.hasFlag("defeated_faculty_leader")) {
        return {"Challenge Math Faculty",
                {"Travel to Viridian and defeat Marius Thaule.",
                 "Bart believes the first faculty still holds one of his sealed records."}};
    }

    if (!player.hasFlag("first_faculty_aftermath_done")) {
        return {"Claim Your Reward",
                {"Speak to Marius after the battle.",
                 "He still has more to tell you about Bart's sealed record."}};
    }

    if (!player.hasFlag("rival_intro_done")) {
        return {"Leave The Faculty",
                {"Step back into Viridian Town.",
                 "Someone outside wants to warn you about Bart Iver."}};
    }

    if (!player.hasFlag("bart_concordance_reveal_done")) {
        return {"Return To Bart",
                {"Bring the Concordance Index back to Bart Iver's lab.",
                 "He promised to explain what the sealed fragment really is."}};
    }

    if (!player.hasFlag("academy_archive_searched")) {
        return {"Search The Archive",
                {"Use the Foundations Badge at Viridian Academy.",
                 "Find the transfer ledger for the next Concordance fragment."}};
    }

    return {"Follow The Transfer",
            {"The archive points toward the Applied Physics Faculty at Aureate Campus.",
             "That is where the next Concordance fragment was sent."}};
}
