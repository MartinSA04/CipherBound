#pragma once
#include <string>
#include "Movement.h"

class Entity: public Movement
{
public:
    Entity(const std::string &name, Position position);
    virtual ~Entity() = default;

    const std::string &getName() const;
    virtual void update() = 0;

protected:
    std::string name;

};
