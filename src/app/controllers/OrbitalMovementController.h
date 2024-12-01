#pragma once
#include <memory>
#include <glm/vec3.hpp>

#include "scene/HmckEntity.h"


namespace Hmck {
    class OrbitalMovementController {
    public:
        void freeOrbit(std::shared_ptr<Entity> entity, const glm::vec3 &center, Window &window, float frameTime,  bool lookAtCenter = true);
        // TODO animate orbit

        // all in radians
        float maxRadius{5.f};
        float minRadius{0.5f};
        float maxElevation{1.5f};
        float minElevation{-1.5f};
        float stepSizeSlow{0.1f};
        float stepSizeNormal{1.0f};

    private:
        float currentRadius{2.f};
        float currentAzimuth{-M_PI / 2.f}; // neg Z
        float currentElevation{0.25f};
        float currentStepSize{0.1};



    };
}
