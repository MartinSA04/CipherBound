#include "../src/save/SaveFormat.h"
#include <cassert>
#include <sstream>

int main() {
    std::istringstream input(R"([header]
name|Ada
map|lab
pos|3|4
facing|2
money|125
respawn_map|house1_2f
respawn_pos|2|3
respawn_facing|1
[flags]
intro_done
[badges]
badge_alpha
[inventory]
5|7
[party]
1;5;10;22;Bulbulum;0;1,2,3,4,5,6;6,5,4,3,2,1;1:15:15,-1:0:0,-1:0:0,-1:0:0
[pc_boxes]
current_box|3
box|3|7;8;80;30;Gravitonion;0;0,0,0,0,0,0;1,1,1,1,1,1;2:10:10,-1:0:0,-1:0:0,-1:0:0
[npcs]
lab|trainer_1|defeated
[daemondex]
seen|1
caught|7
)");

    const SaveFormat::SaveParseResult parsed = SaveFormat::parse(input);
    assert(parsed.warnings.empty());

    assert(parsed.data.playerName == "Ada");
    assert(parsed.data.mapId == "lab");
    assert((parsed.data.position == Position{3, 4}));
    assert(parsed.data.facing == Direction::left);
    assert(parsed.data.money == 125);
    assert(parsed.data.respawnMapId == "house1_2f");
    assert(parsed.data.respawnPosition.has_value());
    assert((*parsed.data.respawnPosition == Position{2, 3}));
    assert(parsed.data.respawnFacing == Direction::down);
    assert(parsed.data.flags.size() == 1);
    assert(parsed.data.badges.size() == 1);
    assert(parsed.data.inventory.size() == 1);
    assert(parsed.data.party.size() == 1);
    assert(parsed.data.currentBox == 3);
    assert(parsed.data.pcBoxes.size() == 4);
    assert(parsed.data.pcBoxes[3].size() == 1);
    assert(parsed.data.npcStates.size() == 1);
    assert(parsed.data.seen.count(1) == 1);
    assert(parsed.data.caught.count(7) == 1);

    const auto &partyDaemon = parsed.data.party[0];
    assert(partyDaemon.speciesId == 1);
    assert(partyDaemon.level == 5);
    assert(partyDaemon.nickname == "Bulbulum");
    assert(partyDaemon.moves[0].moveId == 1);

    const SaveFormat::SaveSlotSummary summary = SaveFormat::summarize(parsed.data);
    assert(summary.playerName == "Ada");
    assert(summary.partySize == 1);
    assert(summary.badgeCount == 1);
    assert(summary.mapId == "lab");
}
