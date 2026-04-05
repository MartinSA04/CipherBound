# Current In-Game Story

This document records only story content that is currently implemented in the game data and story logic.

It should stay conservative: if a story beat is not already supported by current maps, dialogue, cutscenes, or story rules, it does not belong here.

Primary sources for this summary:

- `assets/data/maps/`
- `assets/data/cutscenes/bart_iver_intro.cutscene`
- `src/story/StoryManager.cpp`

## Current Story Shape

The playable opening already has a clear dramatic spine:

- normal student life is interrupted before it properly begins
- Bart Iver acts as a secretive mentor who recruits the player into forbidden research
- the starter choice becomes the player's commitment point
- the first faculty now ends with a concrete reward, a rival confrontation, and a larger reveal
- the second chapter now carries the player through Route 2, Viridian Forest, Pewter Town, Natural Sciences, and the Applied Physics faculty

## Chronological Walkthrough

### 1. Opening State

- The player starts upstairs in their home in the student village.
- There is no full opening narration or cinematic yet. The beginning is driven by exploration and interaction.

### 2. Pallet Student Accommodation

This area currently establishes the tone of the game world.

- A village girl says there are not many students around lately and warns that Bart Iver is a crazy professor who rambles about an ancient language.
- A sign identifies the area as student accommodation.
- An older research sign implies this area used to belong to Prof. Bart Iver's research group before it was turned into a student village.
- The mailbox is the inciting incident:
  - the player's schedule is missing
  - there is an unmarked envelope instead
  - the letter says nobody must see the player talking to Bart
  - the player is asked to come to Bart's lab "just around the corner"
  - the player concludes it sounds important enough to investigate

### 3. Early Story Gates

Before the mailbox is read, the current story logic blocks the player from advancing.

- The north exit to Route 1 is blocked with internal dialogue about needing the schedule first.
- Bart Iver's lab is also blocked before the player has read the letter.

After the mailbox event:

- Bart Iver's lab becomes available.
- Route 1 is still story-gated until Bart's intro has happened and the player has taken a starter.

### 4. Bart Iver Intro Cutscene

The first visit to Bart's lab triggers the main opening cutscene.

Major reveals in the scene:

- Bart has been waiting for the player.
- He introduces himself as Professor Bart Iver.
- He apologizes for the secrecy.
- He says the university leadership fired him and does not want him interacting with students.
- He says they took something important from him.
- He refuses to explain everything immediately, but asks the player to trust him.
- He directly references the player's ambition to find out what Daemons actually are.
- He tells the player to go to the first faculty and retrieve what was taken.
- He warns that the faculty will not give it up without a fight.

This cutscene ends by moving the player to Bart's starter table and setting the `bart_iver_intro_done` flag.

### 5. Starter Selection

Bart presents three rare starter Daemons:

- Bulbulum: the classical Daemon
- Abacub: the algebraic Daemon
- Bugbit: the digital Daemon

Current flow:

- Each Pokeball has a short flavor description.
- Interacting with a Pokeball after the intro scene opens a `Yes / No` choice.
- Accepting a starter gives the player a randomized level 5 Daemon of that species.
- Taking a starter sets the `has_starter` flag.
- Bart then tells the player to take good care of the Daemon, go to the first faculty, and promises that things will be explained later.

After the player already has a starter:

- the chosen Pokeballs become empty
- Bart's repeat dialogue becomes a reminder to go retrieve what was taken from him

### 6. Route 1

Route 1 currently serves as the first journey segment between the student village and Viridian Town.

- There is one trainer here: Field Student.
- The trainer can initiate battle by spotting the player from range.
- If defeated, the trainer switches to defeat dialogue and no longer attacks.

### 7. Viridian Town

Viridian Town now acts as both a hub and the launch point for the second chapter.

- The town connects Route 1 to the first faculty and Route 2.
- After the first faculty aftermath, the rival intercept cutscene triggers here.
- The rival ties Bart's research to their missing older brother.
- The academy archive scene also points the player from Viridian toward Pewter and Applied Physics.

### 8. Viridian Faculty

This is the first proper story destination after receiving the starter.

The faculty contains three trainer NPCs:

- TA Ingrid
- TA Rolf
- TA Liv

Each trainer has a short themed challenge line before battle and a defeat line afterward.

The final trainer is:

- Marius Thaule, the faculty leader

Marius's defeat now triggers a real aftermath cutscene:

- he awards the Foundations Badge
- he hands over the Concordance Index as a key item
- he frames the choice to return it to Bart as a deliberate moral decision

### 9. Bart's First Reveal And The Archive Lead

After the player returns from Viridian Faculty:

- Bart reveals that he and the player's father built the Concordance together
- he names the deeper structure behind Daemons: the Cipher
- the Foundations Badge unlocks Viridian Academy's sealed archive
- the archive identifies the next transfer destination as Applied Physics at Pewter
- the archive also confirms that a senior student vanished during that transfer

### 10. Route 2 And Viridian Forest

The next playable chapter now runs through Route 2 and Viridian Forest.

Current implementation:

- Route 2 has multiple trainer battles instead of serving as an empty connector.
- Viridian Forest now has wild encounters and several trainers of its own.
- The easy east road on Route 2 is physically blocked by Applied Physics staff until the second badge is earned.
- Because of that block, the player must use the forest route on the way to Pewter.

### 11. Pewter Town And Natural Sciences

Pewter Town now acts as the main overworld hub for the Applied Physics chapter.

What is currently implemented:

- Entering Pewter for the first time triggers a rival cutscene that points the player toward Natural Sciences.
- Town NPCs reinforce that the east road and parts of Natural Sciences were sealed after a failed field test.
- The Natural Sciences Building is split across two entrances, with the east side and upper stair blocked off before the second badge.
- A first inspection of the building establishes that the faculty challenge is the only way to gain legitimate access.

### 12. Pewter Faculty

Pewter Faculty now functions as the chapter's main battle gauntlet.

Current implementation:

- three Applied Physics trainers guard the route to the leader
- defeating the leader triggers a cutscene aftermath
- the player receives the Induction Badge there
- the badge explicitly unlocks the sealed sections of Natural Sciences

### 13. Natural Sciences Reveal And Current End

The chapter now continues back into Natural Sciences after the faculty battle.

What is currently implemented:

- the Induction Badge removes the Route 2 shortcut block and opens Natural Sciences
- the upper floor reveal introduces the Resonance Project
- the rival confirms their brother's name appears in the project records
- the player receives the Resonance Ledger as a new key item
- returning to Bart's lab triggers a follow-up reveal
- Bart confirms Applied Physics turned Concordance research into a live field program
- the Resonance Ledger points onward toward a sealed field site beyond campus

The chapter planning docs that informed this implementation are preserved in [story_post_first_faculty.md](story_post_first_faculty.md) and [story_aureate_applied_physics.md](story_aureate_applied_physics.md).

## Implemented Story Flags And Progression

| Flag | How it is set | Current effect |
| --- | --- | --- |
| `read_mail` | Finish the mailbox dialogue | Unlocks Bart's lab, removes the first north-exit story block, changes mailbox repeat dialogue |
| `bart_iver_intro_done` | Reach the end of Bart's intro cutscene | Prevents the cutscene from repeating |
| `has_starter` | Accept one of Bart's starters | Adds the starter to the party, changes Bart/Pokeball dialogue, fully unlocks travel north |
| `defeated_route1_trainer1` | Win the Route 1 trainer battle | Prevents repeat battle and changes that trainer's dialogue |
| `defeated_faculty_trainer1` | Win against TA Ingrid | Prevents repeat battle and changes dialogue |
| `defeated_faculty_trainer2` | Win against TA Rolf | Prevents repeat battle and changes dialogue |
| `defeated_faculty_trainer3` | Win against TA Liv | Prevents repeat battle and changes dialogue |
| `defeated_faculty_leader` | Win against Marius Thaule | Prevents repeat battle and changes dialogue |
| `got_foundations_badge` | Finish the faculty aftermath cutscene | Marks the first badge reward as claimed |
| `received_concordance_index` | Finish the faculty aftermath cutscene | Marks the first research fragment as recovered |
| `first_faculty_aftermath_done` | Finish the faculty aftermath cutscene | Prevents the faculty aftermath from repeating |
| `rival_intro_done` | Finish the rival intercept cutscene | Prevents the rival introduction from repeating |
| `bart_concordance_reveal_done` | Finish Bart's reveal cutscene | Prevents the lab reveal from repeating and updates Bart's dialogue |
| `academy_archive_unlocked` | Finish Bart's reveal cutscene | Unlocks the academy archive story scene |
| `academy_archive_searched` | Finish the academy archive cutscene | Prevents the archive search scene from repeating |
| `aureate_intro_done` | Finish the Pewter arrival cutscene | Marks the Applied Physics chapter as started and updates the objective |
| `natural_sciences_access_denied` | Finish the first Natural Sciences lockdown cutscene | Confirms the player must challenge Pewter Faculty to proceed |
| `defeated_route2_trainer1` | Win the first Route 2 trainer battle | Prevents repeat battle and changes dialogue |
| `defeated_route2_trainer2` | Win the second Route 2 trainer battle | Prevents repeat battle and changes dialogue |
| `defeated_route2_trainer3` | Win the north Route 2 trainer battle | Prevents repeat battle and changes dialogue |
| `defeated_forest_trainer1` | Win the first Viridian Forest trainer battle | Prevents repeat battle and changes dialogue |
| `defeated_forest_trainer2` | Win the second Viridian Forest trainer battle | Prevents repeat battle and changes dialogue |
| `defeated_forest_trainer3` | Win the third Viridian Forest trainer battle | Prevents repeat battle and changes dialogue |
| `defeated_applied_trainer1` | Win against TA Mara | Prevents repeat battle and changes dialogue |
| `defeated_applied_trainer2` | Win against Field Tech Oskar | Prevents repeat battle and changes dialogue |
| `defeated_applied_trainer3` | Win against Researcher Nils | Prevents repeat battle and changes dialogue |
| `defeated_applied_physics_leader` | Win against Dean Solveig | Prevents repeat battle and unlocks the second-badge aftermath |
| `got_induction_badge` | Finish the Applied Physics aftermath cutscene | Opens the sealed Natural Sciences sections and the Route 2 shortcut |
| `resonance_project_discovered` | Finish the Resonance lab reveal cutscene | Marks the hidden project as discovered |
| `received_resonance_ledger` | Finish the Resonance lab reveal cutscene | Adds the Resonance Ledger key item and updates Bart's dialogue |
| `bart_resonance_followup_done` | Finish Bart's Resonance follow-up cutscene | Marks the second chapter's main reveal as complete |

## Current Narrative Gaps

- The player's father is now confirmed as part of Bart's research, but his fate remains unknown.
- The rival's brother is now confirmed inside the Resonance Project trail, but his fate remains unknown.
- The Cipher has been named and partially weaponized by Applied Physics, but not explained in depth.
- The next concrete lead is a sealed field site beyond campus, but that destination is not yet implemented.
