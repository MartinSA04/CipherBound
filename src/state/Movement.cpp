#include "Movement.h"
#include "Map.h"

Movement::Movement(Position position) : position(position), facing(Direction::down) {}

bool Movement::canStep() const
{
    return (!isMoving()) && turnCooldown <= 0;
}

bool Movement::canTurn() const
{
    return !isMoving();
}

bool Movement::canMove(Direction direction, const Map &map) const
{
    Position target = position;
    target.moveDirection(direction);
    return map.isWalkable(target, direction);
}

void Movement::startTurnCooldown()
{
    turnCooldown = 4; // Number of frames to wait after turning before allowing movement
}

void Movement::updateAnimation()
{
    if (turnCooldown > 0)
        --turnCooldown;
    updateWalkAnimation(facing);
}

void Movement::move(Direction direction)
{
    setFacing(direction);
    position.moveDirection(direction);
    startAnimation(direction);
}

Position Movement::getPosition() const { return position; }
void Movement::setPosition(Position pos) { position = pos; }

Direction Movement::getFacing() const { return facing; }
void Movement::setFacing(Direction direction) { facing = direction; }
void Movement::setFacingOpposite(Direction direction)
{
    switch (direction)
    {
    case Direction::up:
        setFacing(Direction::down);
        break;
    case Direction::down:
        setFacing(Direction::up);
        break;
    case Direction::left:
        setFacing(Direction::right);
        break;
    case Direction::right:
        setFacing(Direction::left);
        break;
    }
}