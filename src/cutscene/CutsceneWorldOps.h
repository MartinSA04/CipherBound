#pragma once

#include "../state/World.h"
#include "CutscenePlayback.h"
#include <optional>
#include <string_view>
#include <vector>

namespace CutsceneWorldOps {

void updateAnimations(World &world);
std::optional<Position> tryGetEntityPosition(const World &world, std::string_view targetId);
bool isEntityWalking(const World &world, std::string_view targetId);
Position adjacentDestination(Position origin, Direction direction);
bool setFacing(World &world, std::string_view targetId, Direction direction);
bool setHidden(World &world, std::string_view targetId, bool hidden);
bool stepEntityToward(World &world, std::string_view targetId, Position destination);
void tickMovements(World &world, const std::vector<CutscenePendingMove> &pendingMoves);
bool allMovesComplete(const World &world, const std::vector<CutscenePendingMove> &pendingMoves);

} // namespace CutsceneWorldOps
