#pragma once

#include "HmckGameObject.h"
#include "Platform/HmckWindow.h"
#include "Utils/HmckLogger.h"
#include "Systems/HmckUISystem.h"

#include <limits>

namespace Hmck
{
	class KeyboardMovementController
	{
	public:

        void moveInPlaneXZ(HmckWindow& window, float dt, HmckGameObject& gameObject);

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