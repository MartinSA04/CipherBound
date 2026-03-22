#include "Entity.h"
#include <utility>

Entity::Entity(const std::string &name, Position position) : Movement(position), name(name) {}

const std::string &Entity::getName() const { return name; }

void Entity::setName(std::string newName) { name = std::move(newName); }
