#include "HmckInputHandler.h"

bool Hmck::InputManager::isKeyboardKeyDown(KeyboardKey key)
{
	return glfwGetKey(window, keyboardKeyMap[key]) == GLFW_PRESS;
}

bool Hmck::InputManager::isMouseKeyDown(MouseKey key)
{
	return glfwGetMouseButton(window, mouseKeyMap[key]) == GLFW_PRESS;
}
