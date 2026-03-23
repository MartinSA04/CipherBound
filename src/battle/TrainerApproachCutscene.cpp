#include "TrainerApproachCutscene.h"
#include "../state/NPC.h"
#include "../state/player/Player.h"
#include <cstdlib>

namespace TrainerApproachCutscene {
namespace {

CutsceneStep makeFaceStep(std::string target, Direction direction) {
    CutsceneStep step{};
    step.type = CutsceneStep::Type::face;
    step.target = std::move(target);
    step.direction = direction;
    return step;
}

CutsceneStep makeMoveStep(std::string target, Position destination) {
    CutsceneStep step{};
    step.type = CutsceneStep::Type::move;
    step.target = std::move(target);
    step.x = destination.x;
    step.y = destination.y;
    return step;
}

CutsceneStep makeSyncStep() {
    CutsceneStep step{};
    step.type = CutsceneStep::Type::sync;
    return step;
}

CutsceneStep makeSayStep(std::string speaker, std::vector<std::string> lines) {
    CutsceneStep step{};
    step.type = CutsceneStep::Type::say;
    step.speaker = std::move(speaker);
    step.lines = std::move(lines);
    return step;
}

Direction opposite(Direction direction) {
    switch (direction) {
    case Direction::up:
        return Direction::down;
    case Direction::down:
        return Direction::up;
    case Direction::left:
        return Direction::right;
    case Direction::right:
        return Direction::left;
    }

    return Direction::down;
}

Direction directionToward(Position from, Position to, Direction fallback) {
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;

    if (dx == 0 && dy == 0)
        return fallback;
    if (std::abs(dx) >= std::abs(dy))
        return dx >= 0 ? Direction::right : Direction::left;
    return dy >= 0 ? Direction::down : Direction::up;
}

Position adjacentBattlePosition(Position playerPosition, Direction trainerApproachDirection) {
    playerPosition.moveDirection(opposite(trainerApproachDirection));
    return playerPosition;
}

} // namespace

Cutscene build(const NPC &trainer, const Player &player, bool includePreBattleDialogue) {
    const Direction approachDirection =
        directionToward(trainer.getPosition(), player.getPosition(), trainer.getFacing());
    const Position destination = adjacentBattlePosition(player.getPosition(), approachDirection);

    Cutscene cutscene;
    cutscene.id = "trainer_approach_" + trainer.getId();
    cutscene.steps.push_back(makeFaceStep(trainer.getId(), approachDirection));

    if (trainer.getPosition() != destination) {
        cutscene.steps.push_back(makeMoveStep(trainer.getId(), destination));
        cutscene.steps.push_back(makeSyncStep());
    }

    cutscene.steps.push_back(makeFaceStep("player", opposite(approachDirection)));
    if (includePreBattleDialogue) {
        const std::vector<std::string> &preBattleLines =
            trainer.getDialogueLines(player.getFlags());
        if (!preBattleLines.empty())
            cutscene.steps.push_back(makeSayStep(trainer.getName(), preBattleLines));
    }

    return cutscene;
}

} // namespace TrainerApproachCutscene
