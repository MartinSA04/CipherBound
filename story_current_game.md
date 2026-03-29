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

Viridian Town currently acts more as a gameplay hub than a story-heavy location.

- The town connects the route to the first faculty.
- The Mart and healing center work mechanically.
- The academy, overworld, and NPC house currently have no major story dialogue or events.

### 8. Viridian Faculty

This is the first proper story destination after receiving the starter.

The faculty contains three trainer NPCs:

- TA Ingrid
- TA Rolf
- TA Liv

Each trainer has a short themed challenge line before battle and a defeat line afterward.

The final trainer is:

- Marius Thaule, the faculty leader

Marius currently says the player has earned the faculty badge after defeat, but this is only represented as dialogue right now.

### 9. Current End Of Playable Story

After defeating Marius Thaule, the story now continues in four connected steps.

What is currently implemented:

- Marius awards the Foundations Badge.
- The player receives the Concordance Index as a real key item.
- A rival intercepts the player in Viridian Town and reveals that their older brother vanished while tied to Bart's old research.
- Returning to Bart's lab triggers the first major reveal: Bart and the player's father built the Concordance together.
- Bart identifies the deeper structure behind Daemons as the Cipher.
- Bart sends the player to Viridian Academy's restricted archive.
- The academy archive reveals that the next fragment was transferred to Applied Physics Faculty at Aureate Campus.
- The archive also confirms that a senior student vanished during that transfer.

The detailed scene plan that guided this implementation is documented in [story_post_first_faculty.md](story_post_first_faculty.md).

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

## Current Narrative Gaps

- The player's father is now confirmed as part of Bart's research, but his fate remains unknown.
- The rival's brother is now part of the mystery, but not yet found.
- The Cipher has been named, but not explained in depth.
- The next destination is identified, but not yet implemented as a playable chapter.
