/**
 * @file
 * @brief Direction, position, and movement helpers for world entities.
 * @ingroup world_state
 */

#pragma once
#include "player/WalkAnimation.h"

class Map;

/// Cardinal tile directions used throughout overworld movement and scripting.
enum class Direction {
    up,    ///< Negative Y direction.
    down,  ///< Positive Y direction.
    left,  ///< Negative X direction.
    right, ///< Positive X direction.
};

/**
 * @brief Integer tile coordinate in map space.
 * @ingroup world_state
 */
struct Position {
    int x; ///< Horizontal tile coordinate.
    int y; ///< Vertical tile coordinate.

    /// Returns whether two positions refer to the same tile.
    bool operator==(const Position &other) const { return x == other.x && y == other.y; }
    /// Returns whether two positions refer to different tiles.
    bool operator!=(const Position &other) const { return !(*this == other); }
    /// Moves the position by one tile in the given direction.
    void moveDirection(const Direction &dir) {
        switch (dir) {
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

/**
 * @brief Base class for entities that occupy a tile and can animate movement.
 * @ingroup world_state
 */
class Movement : public WalkAnimation {

  public:
    /// Creates a movement component at the given starting tile.
    Movement(Position position);

    /// Moves one tile in the current world if the caller already validated the step.
    void move(Direction direction);
    /// Returns whether movement in the given direction is currently allowed.
    bool canMove(Direction direction, const Map &map) const;
    /// Returns whether the entity may begin another step this frame.
    bool canStep() const;
    /// Returns whether the entity may turn this frame.
    bool canTurn() const;
    /// Starts the post-turn cooldown that delays movement.
    void startTurnCooldown();
    /// Advances walk animation and cooldown state by one frame.
    void updateAnimation();

    /// Returns the current tile position.
    Position getPosition() const;
    /// Sets the current tile position directly.
    void setPosition(Position position);

    /// Returns the current facing direction.
    Direction getFacing() const;
    /// Sets the current facing direction.
    void setFacing(Direction direction);
    /// Sets the facing direction to the opposite of the input direction.
    void setFacingOpposite(Direction direction);

  protected:
    Position position; ///< Current tile position.
    Direction facing;  ///< Current facing direction.

  private:
    int turnCooldown{0}; ///< Frames to wait after turning before moving.
};
