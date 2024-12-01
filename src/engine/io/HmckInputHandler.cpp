#include "HmckInputHandler.h"

#include <cstdio>


double Hmck::InputHandler::scrollOffset = 0.0;

bool Hmck::InputHandler::isKeyboardKeyDown(const KeyboardKey key) {
    return glfwGetKey(window, keyboardKeyMap[key]) == GLFW_PRESS;
}

bool Hmck::InputHandler::isMouseKeyDown(const MouseKey key) {
    return glfwGetMouseButton(window, mouseKeyMap[key]) == GLFW_PRESS;
}
