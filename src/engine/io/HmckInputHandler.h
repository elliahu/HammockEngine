#pragma once

#include <map>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Hmck {
    enum KeyboardKey {
        KEY_W,
        KEY_S,
        KEY_A,
        KEY_D,
        KEY_E,
        KEY_SPACE,
        KEY_LSHIFT,
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,
        __KEYBOARD_KEY_COUNT__
    };

    enum MouseKey {
        MOUSE_LEFT,
        MOUSE_RIGHT,
        MOUSE_MIDDLE,
        __MOUSE_KEY_COUNT__
    };

    class InputHandler {
    public:
        void setWindow(GLFWwindow *_window) { window = _window; };

        bool isKeyboardKeyDown(KeyboardKey key);

        bool isMouseKeyDown(MouseKey key);


    private:
        static double scrollOffset;

        std::map<KeyboardKey, int> keyboardKeyMap = {
            {KEY_W, GLFW_KEY_W},
            {KEY_A, GLFW_KEY_A},
            {KEY_S, GLFW_KEY_S},
            {KEY_D, GLFW_KEY_D},
            {KEY_E, GLFW_KEY_E},
            {KEY_SPACE, GLFW_KEY_SPACE},
            {KEY_LSHIFT, GLFW_KEY_LEFT_SHIFT},
            {KEY_UP, GLFW_KEY_UP},
            {KEY_DOWN, GLFW_KEY_DOWN},
            {KEY_LEFT, GLFW_KEY_LEFT},
            {KEY_RIGHT, GLFW_KEY_RIGHT},
        };

        std::map<MouseKey, int> mouseKeyMap = {
            {MOUSE_LEFT, GLFW_MOUSE_BUTTON_1},
            {MOUSE_RIGHT, GLFW_MOUSE_BUTTON_2},
            {MOUSE_MIDDLE, GLFW_MOUSE_BUTTON_3},
        };

        GLFWwindow *window;
    };
}
