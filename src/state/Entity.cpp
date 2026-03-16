#include "Entity.h"

Entity::Entity(const std::string &name, Position position)
    : Movement(position), name(name) {}

const std::string &Entity::getName() const { return name; }
