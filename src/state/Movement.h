#pragma once
#include "WalkAnimation.h"

class Map;

enum class Direction
{
    up,
    down,
    left,
    right,
};

struct Position
{
    int x, y;

    bool operator==(const Position &other) const { return x == other.x && y == other.y; }
    bool operator!=(const Position &other) const { return !(*this == other); }
    void moveDirection(const Direction &dir)
    {
        switch (dir)
        {
        case Direction::up:
            y--;
            break;
        case Direction::down:
            y++;
            break;
        case Direction::left:
            x--;
            break;
        case Direction::right:
            x++;
            break;
        }
    }
};

class Movement : public WalkAnimation
{

public:
    Movement(Position position);

    void move(Direction direction);
    bool canMove(Direction direction, const Map &map) const;
    bool canStep() const;
    bool canTurn() const;
    void startTurnCooldown();
    void updateAnimation();

    Position getPosition() const;
    void setPosition(Position position);

    Direction getFacing() const;
    void setFacing(Direction direction);
    void setFacingOpposite(Direction direction);

protected:
    Position position;
    Direction facing;

private:
    int turnCooldown{0}; // frames to wait after turning before moving
};