#pragma once

#include "HmckGameObject.h"
#include "HmckWindow.h"
#include "Utils/HmckLogger.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <limits>

namespace Hmck
{
	class KeyboardMovementController
	{
	public:

        struct KeyMappings {
            int moveLeft = GLFW_KEY_A;
            int moveRight = GLFW_KEY_D;
            int moveForward = GLFW_KEY_W;
            int moveBackward = GLFW_KEY_S;
            int moveUp = GLFW_KEY_SPACE;
            int moveDown = GLFW_KEY_LEFT_SHIFT;
            int lookLeft = GLFW_KEY_LEFT;
            int lookRight = GLFW_KEY_RIGHT;
            int lookUp = GLFW_KEY_UP;
            int lookDown = GLFW_KEY_DOWN;
            int lookAround = GLFW_MOUSE_BUTTON_2;
        };

        KeyboardMovementController(GLFWwindow* window)
        {
            glfwSetMouseButtonCallback(window, mouseButtonCallback);
        }

        void moveInPlaneXZ(GLFWwindow* window, float dt, HmckGameObject& gameObject);

        KeyMappings keys{};
        float moveSpeed{ 3.f };
        float lookSpeed{ 1.5f };
    private:

        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

        static bool mouseMotionStarted;
        static bool mouseMotionEnded;
        bool mouseInMotion = false;
        double mouseMotionStartX;
        double mouseMotionStartY;
        double mouseMotionLastX;
        double mouseMotionLasrY;
	};
}