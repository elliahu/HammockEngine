#include "HmckInputHandler.h"

bool Hmck::InputManager::isKeyboardKeyDown(const KeyboardKey key) {
    return glfwGetKey(window, keyboardKeyMap[key]) == GLFW_PRESS;
}

bool Hmck::InputManager::isMouseKeyDown(const MouseKey key) {
    return glfwGetMouseButton(window, mouseKeyMap[key]) == GLFW_PRESS;
}
