#include "../src/battle/TrainerApproachCutscene.h"
#include "../src/state/NPC.h"
#include "../src/state/player/Player.h"
#include <cassert>

int main() {
    Player player("Player", {5, 5});
    NPC trainer("route1_trainer1", "Bug Catcher", {5, 1}, NPCType::trainer, "bug_catcher");
    trainer.setFacing(Direction::down);
    trainer.addDialogueStage("", {"Hey!", "Let's battle!"});

    const Cutscene cutscene = TrainerApproachCutscene::build(trainer, player, true);
    assert(cutscene.id == "trainer_approach_route1_trainer1");
    assert(cutscene.steps.size() == 5);

    const CutsceneStep &faceTrainer = cutscene.steps[0];
    assert(faceTrainer.type == CutsceneStep::Type::face);
    assert(faceTrainer.target == "route1_trainer1");
    assert(faceTrainer.direction == Direction::down);

    const CutsceneStep &moveTrainer = cutscene.steps[1];
    assert(moveTrainer.type == CutsceneStep::Type::move);
    assert(moveTrainer.target == "route1_trainer1");
    assert(moveTrainer.x == 5);
    assert(moveTrainer.y == 4);

    const CutsceneStep &sync = cutscene.steps[2];
    assert(sync.type == CutsceneStep::Type::sync);

    const CutsceneStep &facePlayer = cutscene.steps[3];
    assert(facePlayer.type == CutsceneStep::Type::face);
    assert(facePlayer.target == "player");
    assert(facePlayer.direction == Direction::up);

    const CutsceneStep &say = cutscene.steps[4];
    assert(say.type == CutsceneStep::Type::say);
    assert(say.speaker == "Bug Catcher");
    assert(say.lines.size() == 2);
    assert(say.lines[0] == "Hey!");

    NPC adjacentTrainer("adjacent", "Lass", {4, 5}, NPCType::trainer, "girl");
    adjacentTrainer.setFacing(Direction::right);
    adjacentTrainer.addDialogueStage("", {"Seen you."});
    const Cutscene adjacentCutscene = TrainerApproachCutscene::build(adjacentTrainer, player);
    assert(adjacentCutscene.steps.size() == 2);
    assert(adjacentCutscene.steps[0].type == CutsceneStep::Type::face);
    assert(adjacentCutscene.steps[1].type == CutsceneStep::Type::face);

    return 0;
}
