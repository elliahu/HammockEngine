#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include "HmckEntity.h"
#include "core/HmckFramebuffer.h"
#include "core/HmckGraphicsPipeline.h"
#include "core/HmckResourceManager.h"

namespace Hmck {
    class ILight : public Entity {
    public:
        ILight() = default;
        ~ILight() override= default;

        float linearTerm = 1.0f;
        float quadraticTerm = 1.0f;
        glm::vec3 color = glm::vec3(1.0f);
    };

    class OmniLight : public ILight {
    public:
        OmniLight() = default;
        ~OmniLight() override= default;


    };
}
