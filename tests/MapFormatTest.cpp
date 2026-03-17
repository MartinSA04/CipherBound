#include "../src/state/world/MapFormat.h"
#include <cassert>
#include <sstream>

int main() {
    std::istringstream input(R"([header]
id|test_map
width|4
height|3
background|bg.png
background_overlay|overlay.png
player_spawn|1|2
[tiles]
....
.##.
....
[warps]
1|2|next_map|3|4
[encounters]
1|2|4|30
broken|entry
[npcs]
guide|Guide|normal|1|1|left|0|hello there@@quest_done?thanks for helping|villager_f|
trainer_1|Trainer|trainer|2|2|up|4|Ready?;Let's battle!|bart_iver|1:5,7:8
)");

    const MapFormat::ParseResult parsed = MapFormat::parse(input);
    assert(parsed.valid());
    assert(parsed.warnings.size() == 1);

    const auto &definition = parsed.definition;
    assert(definition.id == "test_map");
    assert(definition.width == 4);
    assert(definition.height == 3);
    assert(definition.backgroundImage == "bg.png");
    assert(definition.backgroundImageOverlay == "overlay.png");
    assert(definition.playerSpawn.has_value());
    assert((*definition.playerSpawn == Position{1, 2}));
    assert(definition.tileRows.size() == 3);
    assert(definition.warps.size() == 1);
    assert(definition.encounters.size() == 1);
    assert(definition.npcs.size() == 2);

    const auto &guide = definition.npcs[0];
    assert(guide.id == "guide");
    assert(guide.spriteType == "villager_f");
    assert(guide.type == NPCType::normal);
    assert(guide.facing == Direction::left);
    assert(guide.dialogueStages.size() == 2);
    assert(guide.dialogueStages[0].requiredFlag.empty());
    assert(guide.dialogueStages[0].lines.size() == 1);
    assert(guide.dialogueStages[1].requiredFlag == "quest_done");

    const auto &trainer = definition.npcs[1];
    assert(trainer.spriteType == "bart_iver");
    assert(trainer.type == NPCType::trainer);
    assert(trainer.sightRange == 4);
    assert(trainer.party.size() == 2);
    assert(trainer.party[0].speciesId == 1);
    assert(trainer.party[1].level == 8);
}
