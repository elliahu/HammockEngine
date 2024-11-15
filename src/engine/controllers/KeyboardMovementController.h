#pragma once

#include "platforms/HmckWindow.h"
#include "systems/HmckUserInterface.h"

#include <memory>

namespace Hmck {
    class KeyboardMovementController {
    public:
        void moveInPlaneXZ(Window &window, float dt, std::shared_ptr<Entity> entity);

        float moveSpeed{3.f};
        float lookSpeed{1.5f};

    private:
        bool mouseInMotion = false;
        double mouseMotionStartX = 0;
        double mouseMotionStartY = 0;
        double mouseMotionLastX = 0;
        double mouseMotionLasrY = 0;
    };
}
