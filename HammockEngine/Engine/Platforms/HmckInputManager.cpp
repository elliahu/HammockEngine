#include "HmckInputmanager.h"

void Hmck::HmckInputManager::addEvent(SDL_Event event)
{
	handleEvent(event);
	eventStack.push(event);
}

void Hmck::HmckInputManager::clearEvents()
{
	while (!eventStack.empty()) {
		eventStack.pop();
	}
}

bool Hmck::HmckInputManager::isKeyboardKeyDown(HmckKeyboardKey key)
{
	return keyboardState[key];
}

bool Hmck::HmckInputManager::isMouseKeyDown(HmckMouseKey key)
{
	return mouseState[key];
}

void Hmck::HmckInputManager::handleEvent(SDL_Event& event)
{	
	if (event.type == SDL_KEYDOWN) 
	{
		setKeyboardState(event.key.keysym.scancode, true);
	}
	else if (event.type == SDL_KEYUP)
	{
		setKeyboardState(event.key.keysym.scancode, false);
	}
	else if (event.type == SDL_MOUSEBUTTONDOWN)
	{
		setMouseState(event.button.button, true);
	}
	else if (event.type == SDL_MOUSEBUTTONUP)
	{
		setMouseState(event.button.button, false);
	}
}

void Hmck::HmckInputManager::setKeyboardState(SDL_Scancode scancode, bool state)
{
	switch (scancode)
	{
	case SDL_SCANCODE_W:
		keyboardState[HMCK_KEY_W] = state;
		break;
	case SDL_SCANCODE_A:
		keyboardState[HMCK_KEY_A] = state;
		break;
	case SDL_SCANCODE_S:
		keyboardState[HMCK_KEY_S] = state;
		break;
	case SDL_SCANCODE_D:
		keyboardState[HMCK_KEY_D] = state;
		break;
	case SDL_SCANCODE_SPACE:
		keyboardState[HMCK_KEY_SPACE] = state;
		break;
	case SDL_SCANCODE_LSHIFT:
		keyboardState[HMCK_KEY_LSHIFT] = state;
		break;
	case SDL_SCANCODE_UP:
		keyboardState[HMCK_KEY_UP] = state;
		break;
	case SDL_SCANCODE_DOWN:
		keyboardState[HMCK_KEY_DOWN] = state;
		break;
	case SDL_SCANCODE_LEFT:
		keyboardState[HMCK_KEY_LEFT] = state;
		break;
	case SDL_SCANCODE_RIGHT:
		keyboardState[HMCK_KEY_RIGHT] = state;
		break;
	default:
		break;
	}
}

void Hmck::HmckInputManager::setMouseState(Uint8 key, bool state)
{
	if (key == SDL_BUTTON_LEFT) 
	{
		mouseState[HMCK_MOUSE_LEFT] = state;
	}
	else if (key == SDL_BUTTON_RIGHT)
	{
		mouseState[HMCK_MOUSE_RIGHT] = state;
	}
	else if (key == SDL_BUTTON_MIDDLE)
	{
		mouseState[HMCK_MOUSE_MIDDLE] = state;
	}
}

