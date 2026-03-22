#pragma once
#include "Movement.h"
#include <string>

class Entity : public Movement {
  public:
    Entity(const std::string &name, Position position);
    virtual ~Entity() = default;

    const std::string &getName() const;
    void setName(std::string newName);
    virtual void update() = 0;

  protected:
    std::string name;
};
