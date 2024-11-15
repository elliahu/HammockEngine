#pragma once

#include <map>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Hmck {
    enum KeyboardKey {
        HMCK_KEY_W,
        HMCK_KEY_S,
        HMCK_KEY_A,
        HMCK_KEY_D,
        HMCK_KEY_E,
        HMCK_KEY_SPACE,
        HMCK_KEY_LSHIFT,
        HMCK_KEY_UP,
        HMCK_KEY_DOWN,
        HMCK_KEY_LEFT,
        HMCK_KEY_RIGHT,
        __HMCK_KEYBOARD_KEY_COUNT__
    };

    enum MouseKey {
        HMCK_MOUSE_LEFT,
        HMCK_MOUSE_RIGHT,
        HMCK_MOUSE_MIDDLE,
        __HMCK_MOUSE_KEY_COUNT__
    };

    class InputManager {
    public:
        void setWindow(GLFWwindow *_window) { window = _window; };

        bool isKeyboardKeyDown(KeyboardKey key);

        bool isMouseKeyDown(MouseKey key);

    private:
        std::map<KeyboardKey, int> keyboardKeyMap = {
            {HMCK_KEY_W, GLFW_KEY_W},
            {HMCK_KEY_A, GLFW_KEY_A},
            {HMCK_KEY_S, GLFW_KEY_S},
            {HMCK_KEY_D, GLFW_KEY_D},
            {HMCK_KEY_E, GLFW_KEY_E},
            {HMCK_KEY_SPACE, GLFW_KEY_SPACE},
            {HMCK_KEY_LSHIFT, GLFW_KEY_LEFT_SHIFT},
            {HMCK_KEY_UP, GLFW_KEY_UP},
            {HMCK_KEY_DOWN, GLFW_KEY_DOWN},
            {HMCK_KEY_LEFT, GLFW_KEY_LEFT},
            {HMCK_KEY_RIGHT, GLFW_KEY_RIGHT},
        };

        std::map<MouseKey, int> mouseKeyMap = {
            {HMCK_MOUSE_LEFT, GLFW_MOUSE_BUTTON_1},
            {HMCK_MOUSE_RIGHT, GLFW_MOUSE_BUTTON_2},
            {HMCK_MOUSE_MIDDLE, GLFW_MOUSE_BUTTON_3},
        };

        GLFWwindow *window;
    };
}
