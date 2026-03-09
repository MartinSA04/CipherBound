#pragma once
#include <string>
#include "Map.h"

class Entity
{
public:
    Entity(const std::string &name, Position position);
    virtual ~Entity() = default;

    const std::string &getName() const;
    Position getPosition() const;
    void setPosition(Position position);

    Direction getFacing() const;
    void setFacing(Direction direction);
    void setFacingOpposite(Direction direction);

    virtual void update() = 0;

protected:
    std::string name;
    Position position;
    Direction facing;
};
