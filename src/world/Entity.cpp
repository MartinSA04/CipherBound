#include "Entity.h"

Entity::Entity(const std::string &name, Position position)
    : name(name), position(position), facing(Direction::down)
{
}

const std::string &Entity::getName() const { return name; }
Position Entity::getPosition() const { return position; }
void Entity::setPosition(Position pos) { position = pos; }
Direction Entity::getFacing() const { return facing; }
void Entity::setFacing(Direction direction) { facing = direction; }
void Entity::setFacingOpposite(Direction direction)
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
