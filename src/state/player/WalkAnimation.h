/**
 * @file
 * @brief Generic tile-walk animation state shared by movable entities.
 * @ingroup world_state
 */

#pragma once

enum class Direction;
struct Position;

/**
 * @brief Maintains sprite offsets and frame counters for tile-based walking.
 * @ingroup world_state
 */
class WalkAnimation {
  public:
    /// Returns the number of frames used for one tile movement.
    int getMoveDelay() const;
    /// Sets the number of frames used for one tile movement.
    void setMoveDelay(int delay);

    /// Starts a movement animation in the given direction.
    void startAnimation(const Direction &direction);
    /// Returns the current animation frame index.
    int getAnimationFrame();

    /// Returns whether a walk animation is currently in progress.
    bool isMoving() const;
    /// Returns whether movement was active on the previous frame.
    bool wasRecentlyMoving() const;

    /// Returns the horizontal pixel offset used while animating.
    int getPixelOffsetX() const;
    /// Returns the vertical pixel offset used while animating.
    int getPixelOffsetY() const;
    /// Returns the walk-cycle frame used for sprite selection.
    int getWalkFrame() const;

  protected:
    /// Advances offsets and walk-cycle state for one frame.
    void updateWalkAnimation(const Direction &facing);

    int pixelOffsetX{0}; ///< Current horizontal pixel offset during animation.
    int pixelOffsetY{0}; ///< Current vertical pixel offset during animation.
    int animFramesLeft{0}; ///< Remaining frames in the active movement animation.
    int moveDelay{12};     ///< Frames per tile movement.
    int walkFrame{0};      ///< Walk-cycle counter used for alternating step sprites.
    bool wasMoving{false}; ///< Whether movement was active on the previous frame.
};
