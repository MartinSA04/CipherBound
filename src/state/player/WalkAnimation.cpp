#include "WalkAnimation.h"
#include "../../ui/UiConstants.h"
#include "../Movement.h"

int WalkAnimation::getMoveDelay() const { return moveDelay; }
void WalkAnimation::setMoveDelay(int delay) { moveDelay = (delay <= 0) ? 1 : delay; }
int WalkAnimation::getAnimationFrame() { return animFramesLeft; }
int WalkAnimation::getPixelOffsetX() const { return pixelOffsetX; }
int WalkAnimation::getPixelOffsetY() const { return pixelOffsetY; }
int WalkAnimation::getWalkFrame() const { return walkFrame; }

void WalkAnimation::startAnimation(const Direction &direction) {
    animFramesLeft = moveDelay;
    walkFrame++;
    switch (direction) {
    case Direction::up:
        pixelOffsetX = 0;
        pixelOffsetY = TILE_SIZE;
        break;
    case Direction::down:
        pixelOffsetX = 0;
        pixelOffsetY = -TILE_SIZE;
        break;
    case Direction::left:
        pixelOffsetX = TILE_SIZE;
        pixelOffsetY = 0;
        break;
    case Direction::right:
        pixelOffsetX = -TILE_SIZE;
        pixelOffsetY = 0;
        break;
    }
}

bool WalkAnimation::isMoving() const { return animFramesLeft > 0; }

bool WalkAnimation::wasRecentlyMoving() const { return wasMoving; }

void WalkAnimation::updateWalkAnimation(const Direction &facing) {
    if (!isMoving()) {
        wasMoving = false;
        return;
    }

    wasMoving = true;
    --animFramesLeft;

    if (animFramesLeft == moveDelay / 2)
        walkFrame++;

    if (!isMoving()) {
        // Snap to grid
        pixelOffsetX = 0;
        pixelOffsetY = 0;
    } else {
        // Linearly interpolate: offset goes from full tile toward 0
        // Total pixels to cover = TILE_SIZE, over moveDelay frames
        double t = static_cast<double>(animFramesLeft) / static_cast<double>(moveDelay);
        int totalOffset = static_cast<int>(t * TILE_SIZE);

        switch (facing) {
        case Direction::up:
            pixelOffsetX = 0;
            pixelOffsetY = totalOffset;
            break;
        case Direction::down:
            pixelOffsetX = 0;
            pixelOffsetY = -totalOffset;
            break;
        case Direction::left:
            pixelOffsetX = totalOffset;
            pixelOffsetY = 0;
            break;
        case Direction::right:
            pixelOffsetX = -totalOffset;
            pixelOffsetY = 0;
            break;
        }
    }
}
