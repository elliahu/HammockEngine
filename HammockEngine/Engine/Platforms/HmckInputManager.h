#pragma once

#include <stack>
#include <vector>
#include <SDL.h>

namespace Hmck 
{
	enum HmckKeyboardKey
	{
		HMCK_KEY_W,
		HMCK_KEY_S,
		HMCK_KEY_A,
		HMCK_KEY_D,
		HMCK_KEY_SPACE,
		HMCK_KEY_LSHIFT,
		HMCK_KEY_UP,
		HMCK_KEY_DOWN,
		HMCK_KEY_LEFT,
		HMCK_KEY_RIGHT,
		__HMCK_KEYBOARD_KEY_COUNT__
	};

	enum HmckMouseKey
	{
		HMCK_MOUSE_LEFT,
		HMCK_MOUSE_RIGHT,
		HMCK_MOUSE_MIDDLE,
		__HMCK_MOUSE_KEY_COUNT__
	};


	class HmckInputManager
	{
	public:
		void addEvent(SDL_Event event);
		void clearEvents();
		bool isKeyboardKeyDown(HmckKeyboardKey key);
		bool isMouseKeyDown(HmckMouseKey key);

	private:
		void handleEvent(SDL_Event& event);
		void setKeyboardState(SDL_Scancode scancode, bool state);
		void setMouseState(Uint8 key, bool state);
		
		bool keyboardState[__HMCK_KEYBOARD_KEY_COUNT__ + 1] = { false };
		bool mouseState[__HMCK_MOUSE_KEY_COUNT__ + 1] = { false };
		std::stack<SDL_Event> eventStack{};
	};
}
