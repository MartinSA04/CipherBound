#include "InputManager.h"

InputManager::InputManager(TDT4102::AnimationWindow &window)
    : window(window), prevConfirm(false), prevCancel(false), prevMenu(false) {}

void InputManager::update() {
    // Store previous state for edge detection
    prevConfirm = isConfirmHeld();
    prevCancel = isCancelHeld();
    prevMenu = isMenuHeld();
}

bool InputManager::isUpHeld() const {
    return window.is_key_down(KeyboardKey::UP) ||
           window.is_key_down(KeyboardKey::W);
}

bool InputManager::isDownHeld() const {
    return window.is_key_down(KeyboardKey::DOWN) ||
           window.is_key_down(KeyboardKey::S);
}

bool InputManager::isLeftHeld() const {
    return window.is_key_down(KeyboardKey::LEFT) ||
           window.is_key_down(KeyboardKey::A);
}

bool InputManager::isRightHeld() const {
    return window.is_key_down(KeyboardKey::RIGHT) ||
           window.is_key_down(KeyboardKey::D);
}

bool InputManager::getMovementDirection(Direction &outDirection) const {
    if (isUpHeld()) {
        outDirection = Direction::up;
        return true;
    }
    if (isDownHeld()) {
        outDirection = Direction::down;
        return true;
    }
    if (isLeftHeld()) {
        outDirection = Direction::left;
        return true;
    }
    if (isRightHeld()) {
        outDirection = Direction::right;
        return true;
    }
    return false;
}

bool InputManager::isConfirmHeld() const {
    return window.is_key_down(KeyboardKey::Z) ||
           window.is_key_down(KeyboardKey::ENTER);
}

bool InputManager::isCancelHeld() const {
    return window.is_key_down(KeyboardKey::X) ||
           window.is_key_down(KeyboardKey::ESCAPE);
}

bool InputManager::isMenuHeld() const {
    return window.is_key_down(KeyboardKey::C) ||
           window.is_key_down(KeyboardKey::BACKSPACE);
}

bool InputManager::isRunHeld() const {
    return window.is_key_down(KeyboardKey::LEFT_SHIFT);
}

bool InputManager::isConfirmPressed() const {
    return isConfirmHeld() && !prevConfirm;
}

bool InputManager::isCancelPressed() const {
    return isCancelHeld() && !prevCancel;
}

bool InputManager::isMenuPressed() const { return isMenuHeld() && !prevMenu; }

bool InputManager::isKeyHeld(KeyboardKey key) const {
    return window.is_key_down(key);
}
