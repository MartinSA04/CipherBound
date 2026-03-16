#pragma once
#include "../state/Movement.h"
#include <AnimationWindow.h>
#include <KeyboardKey.h>

class InputManager {
  public:
    InputManager(TDT4102::AnimationWindow &window);

    void update();

    bool isUpHeld() const;
    bool isDownHeld() const;
    bool isLeftHeld() const;
    bool isRightHeld() const;

    bool getMovementDirection(Direction &outDirection) const;

    bool isConfirmHeld() const;
    bool isCancelHeld() const;
    bool isMenuHeld() const;
    bool isRunHeld() const;

    bool isConfirmPressed() const;
    bool isCancelPressed() const;
    bool isMenuPressed() const;

    bool isKeyHeld(KeyboardKey key) const;

  private:
    TDT4102::AnimationWindow &window;

    bool prevConfirm;
    bool prevCancel;
    bool prevMenu;
};
