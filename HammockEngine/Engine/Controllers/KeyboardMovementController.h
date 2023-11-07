#pragma once

#include "Platform/HmckWindow.h"
#include "Utils/HmckLogger.h"
#include "Systems/HmckUserInterface.h"

#include <limits>
#include <memory>

namespace Hmck
{
	class KeyboardMovementController
	{
	public:

        void moveInPlaneXZ(Window& window, float dt, std::shared_ptr<Entity> entity);

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