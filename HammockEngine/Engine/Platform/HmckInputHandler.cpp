#include "HmckInputHandler.h"

bool Hmck::HmckInputManager::isKeyboardKeyDown(HmckKeyboardKey key)
{
	return glfwGetKey(window, keyboardKeyMap[key]) == GLFW_PRESS;
}

bool Hmck::HmckInputManager::isMouseKeyDown(HmckMouseKey key)
{
	return glfwGetMouseButton(window, mouseKeyMap[key]) == GLFW_PRESS;
}
