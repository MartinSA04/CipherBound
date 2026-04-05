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
constexpr const char *pewterArrivalCutscenePath =
    "assets/data/cutscenes/pewter_arrival.cutscene";
constexpr const char *naturalSciencesLockdownCutscenePath =
    "assets/data/cutscenes/natural_sciences_lockdown.cutscene";
constexpr const char *appliedPhysicsAftermathCutscenePath =
    "assets/data/cutscenes/applied_physics_aftermath.cutscene";
constexpr const char *resonanceLabRevealCutscenePath =
    "assets/data/cutscenes/resonance_lab_reveal.cutscene";
constexpr const char *bartResonanceFollowupCutscenePath =
    "assets/data/cutscenes/bart_resonance_followup.cutscene";

void setHiddenIfPresent(World &world, const std::string &mapId, const char *npcId, bool hidden) {
    if (NPC *npc = world.findNPCById(mapId, npcId); npc != nullptr)
        npc->setHidden(hidden);
}

void applyMapStoryState(World &world) {
    Player &player = world.getPlayer();
    const std::string &mapId = world.getCurrentMapId();

    if (mapId == "route_2") {
        const bool shortcutOpen = player.hasFlag("got_induction_badge");
        setHiddenIfPresent(world, mapId, "route2_shortcut_guard", shortcutOpen);
        setHiddenIfPresent(world, mapId, "route2_shortcut_aide", shortcutOpen);
    }

    if (mapId == "pewter_town") {
        const bool buildingUnlocked = player.hasFlag("got_induction_badge");
        setHiddenIfPresent(world, mapId, "ns_right_guard", buildingUnlocked);
    }

    if (mapId == "natural_sciences_building") {
        const bool buildingUnlocked = player.hasFlag("got_induction_badge");
        setHiddenIfPresent(world, mapId, "ns_stair_guard", buildingUnlocked);
    }

    if (mapId == "natural_sciences_building_2f") {
        setHiddenIfPresent(world, mapId, "ns_rival", true);
    }
}

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

    if (npc->getId() == "nat_sci_bookshelf_6" && world.getPlayer().hasFlag("got_induction_badge") &&
        !world.getPlayer().hasFlag("natural_sciences_reference_found")) {
        world.getPlayer().setFlag("natural_sciences_reference_found");
    }

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
             "Take care of it, and it will carry you farther than any lecture.",
             "Go to the first faculty and bring back anything sealed with my mark.",
             "When you return, I'll tell you why they were so eager to bury the notation."});
    }

    // Default: return to previous state
    return StoryAction::returnToState();
}

StoryAction StoryManager::checkWarp(const WarpPoint &warp, Player &player) {

    if (warp.targetMapId == "route_1") {
        if (!player.hasFlag("read_mail")) {
            return StoryAction::blockWarp(
                {"I should check my mailbox first.", "My schedule should be there."});
        }

        if (!player.hasFlag("bart_iver_intro_done")) {
            return StoryAction::blockWarp(
                {"I should hear Bart out before heading north.",
                 "If he risked sending that letter, there was a reason."});
        }

        if (!player.hasFlag("has_starter")) {
            return StoryAction::blockWarp({"I should choose a Daemon first."});
        }
    }

    if (warp.targetMapId == "bart_iver_lab") {
        if (!player.hasFlag("read_mail")) {
            return StoryAction::blockWarp(
                {"I should not walk into Bart Iver's lab without a reason."});
        }
    }

    if (warp.targetMapId == "route_2" && !player.hasFlag("academy_archive_searched")) {
        return StoryAction::blockWarp(
            {"Bart needs the academy transfer record first.",
             "I should not head for Route 2 until I know where the next fragment went."});
    }

    if (warp.targetMapId == "pewter_faculty" && player.hasFlag("academy_archive_searched") &&
        !player.hasFlag("natural_sciences_access_denied")) {
        return StoryAction::blockWarp(
            {"The archive pointed to the Natural Sciences Building, not the faculty hall.",
             "I should inspect the transfer site before I challenge anyone here."});
    }

    if (warp.targetMapId == "natural_sciences_building_2f" &&
        player.hasFlag("got_induction_badge") &&
        !player.hasFlag("natural_sciences_reference_found") &&
        !player.hasFlag("received_resonance_ledger")) {
        return StoryAction::blockWarp(
            {"The upstairs archive is too broad to search blind.",
             "The east-wing project index should narrow the cabinet down."});
    }

    return StoryAction::none();
}

StoryAction StoryManager::checkMapEnter(World &world) {
    applyMapStoryState(world);

    Player &player = world.getPlayer();
    const std::string &mapId = world.getCurrentMapId();

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

    if (mapId == "pewter_town" && player.hasFlag("academy_archive_searched") &&
        !player.hasFlag("aureate_intro_done")) {
        return StoryAction::startCutscene(pewterArrivalCutscenePath);
    }

    if (mapId == "natural_sciences_building" && player.hasFlag("aureate_intro_done") &&
        !player.hasFlag("natural_sciences_access_denied") &&
        !player.hasFlag("got_induction_badge")) {
        return StoryAction::startCutscene(naturalSciencesLockdownCutscenePath);
    }

    if (mapId == "pewter_faculty" && player.hasFlag("defeated_applied_physics_leader") &&
        !player.hasFlag("got_induction_badge")) {
        return StoryAction::startCutscene(appliedPhysicsAftermathCutscenePath);
    }

    if (mapId == "natural_sciences_building_2f" && player.hasFlag("got_induction_badge") &&
        player.hasFlag("natural_sciences_reference_found") &&
        !player.hasFlag("received_resonance_ledger")) {
        return StoryAction::startCutscene(resonanceLabRevealCutscenePath);
    }

    if (mapId == "bart_iver_lab" && player.hasFlag("received_resonance_ledger") &&
        !player.hasFlag("bart_resonance_followup_done")) {
        return StoryAction::startCutscene(bartResonanceFollowupCutscenePath);
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

    if (!player.hasFlag("aureate_intro_done")) {
        return {"Reach Aureate Campus",
                {"Follow the transfer north through Route 2.",
                 "The next Concordance fragment was sent to Applied Physics at Pewter."}};
    }

    if (!player.hasFlag("natural_sciences_access_denied")) {
        return {"Investigate Natural Sciences",
                {"The transfer trail leads into Pewter's Natural Sciences Building.",
                 "See what Applied Physics is hiding there."}};
    }

    if (!player.hasFlag("defeated_applied_physics_leader")) {
        return {"Earn The Induction Badge",
                {"Natural Sciences is sealed without Pewter Faculty authorization.",
                 "Defeat the Applied Physics faculty leader to force the building open."}};
    }

    if (!player.hasFlag("got_induction_badge")) {
        return {"Claim The Induction Badge",
                {"The faculty leader still has more to say after the battle.",
                 "Take the badge that opens the sealed Natural Sciences sections."}};
    }

    if (!player.hasFlag("natural_sciences_reference_found")) {
        return {"Search The Project Index",
                {"The Induction Badge opened Natural Sciences.",
                 "Search the sealed east wing for the Resonance project reference index."}};
    }

    if (!player.hasFlag("received_resonance_ledger")) {
        return {"Check Annex B Upstairs",
                {"The east-wing index pointed to Annex B, cabinet 3.",
                 "Use the Natural Sciences stair and search the second-floor archive."}};
    }

    if (!player.hasFlag("bart_resonance_followup_done")) {
        return {"Return To Bart",
                {"Bring the Resonance Ledger back to Bart Iver's lab.",
                 "He needs to explain what Applied Physics was doing with the Concordance."}};
    }

    return {"Decode The Ledger",
            {"Bart confirmed Aureate turned the Concordance into the Resonance Project.",
             "The next step lies deeper in the trail the university keeps sealing off."}};
}
