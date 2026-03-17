#include "CutsceneWorldOps.h"
#include <set>

namespace CutsceneWorldOps {
namespace {

NPC *findNPC(World &world, std::string_view targetId) {
    return world.findNPCById(world.getCurrentMapId(), std::string(targetId));
}

const NPC *findNPC(const World &world, std::string_view targetId) {
    for (const auto &npc : world.getNPCs(world.getCurrentMapId())) {
        if (npc->getId() == targetId)
            return npc.get();
    }
    return nullptr;
}

} // namespace

void updateAnimations(World &world) {
    world.getPlayer().updateAnimation();
    for (auto &npc : world.getNPCs(world.getCurrentMapId()))
        npc->updateAnimation();
}

std::optional<Position> tryGetEntityPosition(const World &world, std::string_view targetId) {
    if (targetId == "player")
        return world.getPlayer().getPosition();

    if (const NPC *npc = findNPC(world, targetId); npc != nullptr)
        return npc->getPosition();
    return std::nullopt;
}

bool isEntityWalking(const World &world, std::string_view targetId) {
    if (targetId == "player")
        return world.getPlayer().isMoving();

    if (const NPC *npc = findNPC(world, targetId); npc != nullptr)
        return npc->isMoving();
    return false;
}

Position adjacentDestination(Position origin, Direction direction) {
    origin.moveDirection(direction);
    return origin;
}

bool setFacing(World &world, std::string_view targetId, Direction direction) {
    if (targetId == "player") {
        world.getPlayer().setFacing(direction);
        return true;
    }

    if (NPC *npc = findNPC(world, targetId); npc != nullptr) {
        npc->setFacing(direction);
        return true;
    }
    return false;
}

bool setHidden(World &world, std::string_view targetId, bool hidden) {
    if (NPC *npc = findNPC(world, targetId); npc != nullptr) {
        npc->setHidden(hidden);
        return true;
    }
    return false;
}

bool stepEntityToward(World &world, std::string_view targetId, Position destination) {
    const auto currentPosition = tryGetEntityPosition(world, targetId);
    if (!currentPosition.has_value())
        return false;

    Direction direction = Direction::down;
    if (currentPosition->x < destination.x)
        direction = Direction::right;
    else if (currentPosition->x > destination.x)
        direction = Direction::left;
    else if (currentPosition->y < destination.y)
        direction = Direction::down;
    else if (currentPosition->y > destination.y)
        direction = Direction::up;
    else
        return true;

    if (targetId == "player") {
        Player &player = world.getPlayer();
        Map &map = world.getMap(world.getCurrentMapId());
        map.setOccupied(player.getPosition(), false);
        player.move(direction);
        map.setOccupied(player.getPosition(), true);
        return true;
    }

    if (NPC *npc = findNPC(world, targetId); npc != nullptr) {
        npc->setMoveDelay(24);
        npc->move(direction);
        return true;
    }
    return false;
}

void tickMovements(World &world, const std::vector<CutscenePendingMove> &pendingMoves) {
    std::set<std::string> steppedTargets;

    for (const auto &move : pendingMoves) {
        if (steppedTargets.contains(move.targetId))
            continue;

        if (isEntityWalking(world, move.targetId)) {
            steppedTargets.insert(move.targetId);
            continue;
        }

        const auto currentPosition = tryGetEntityPosition(world, move.targetId);
        if (!currentPosition.has_value())
            continue;
        if (*currentPosition == move.destination)
            continue;

        stepEntityToward(world, move.targetId, move.destination);
        steppedTargets.insert(move.targetId);
    }
}

bool allMovesComplete(const World &world, const std::vector<CutscenePendingMove> &pendingMoves) {
    for (const auto &move : pendingMoves) {
        if (isEntityWalking(world, move.targetId))
            return false;

        const auto currentPosition = tryGetEntityPosition(world, move.targetId);
        if (!currentPosition.has_value() || *currentPosition != move.destination)
            return false;
    }
    return true;
}

} // namespace CutsceneWorldOps
