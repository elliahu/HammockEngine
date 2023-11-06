#pragma once

#include "HmckGameObject.h"
#include "Platform/HmckWindow.h"
#include "Utils/HmckLogger.h"
#include "Systems/HmckUserInterface.h"

#include <limits>

namespace Hmck
{
	class KeyboardMovementController
	{
	public:

        void moveInPlaneXZ(Window& window, float dt, GameObject& gameObject);

        float moveSpeed{ 3.f };
        float lookSpeed{ 1.5f };
    private:
        bool mouseInMotion = false;
        double mouseMotionStartX;
        double mouseMotionStartY;
        double mouseMotionLastX;
        double mouseMotionLasrY;
	};
}