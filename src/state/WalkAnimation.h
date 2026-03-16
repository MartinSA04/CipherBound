#pragma once
// forward declare
enum class Direction;
struct Position;

class WalkAnimation {
  public:
    int getMoveDelay() const;
    void setMoveDelay(int delay);

    // Animation
    void startAnimation(const Direction &direction);
    int getAnimationFrame();

    bool isMoving() const;
    bool wasRecentlyMoving() const;

    int getPixelOffsetX() const;
    int getPixelOffsetY() const;
    int getWalkFrame() const;

  protected:
    void updateWalkAnimation(const Direction &facing);

    int pixelOffsetX{0}; // current pixel offset during animation
    int pixelOffsetY{0};
    int animFramesLeft{0}; // remaining animation frames
    int moveDelay{12};     // frames per tile movement (animation duration)
    int walkFrame{0};      // walk cycle counter (alternates feet each step)
    bool wasMoving; // was the player walking last frame (for turn cooldown
                    // logic)
};
