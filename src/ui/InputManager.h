#pragma once
#include "../state/Movement.h"
#include <AnimationWindow.h>
#include <KeyboardKey.h>

class InputManager {
  public:
    InputManager(TDT4102::AnimationWindow &window);

    // Call once per frame to update input state
    void update();

    // Directional input (returns true on key held)
    bool isUpHeld() const;
    bool isDownHeld() const;
    bool isLeftHeld() const;
    bool isRightHeld() const;

    // Get movement direction (returns true if any direction is pressed)
    bool getMovementDirection(Direction &outDirection) const;

    // Action buttons
    bool isConfirmHeld() const; // Z / Enter
    bool isCancelHeld() const;  // X / Escape
    bool isMenuHeld() const;    // C / Backspace
    bool isRunHeld() const;     // Left Shift

    // Single-press detection (true only on the frame the key goes down)
    bool isConfirmPressed() const;
    bool isCancelPressed() const;
    bool isMenuPressed() const;

    bool isKeyHeld(KeyboardKey key) const;

  private:
    TDT4102::AnimationWindow &window;

    // Previous frame state for press detection
    bool prevConfirm;
    bool prevCancel;
    bool prevMenu;
};
