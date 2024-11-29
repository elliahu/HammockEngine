#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace Hmck {
    struct Vertex {
        glm::vec3 position{};
        glm::vec3 color{};
        glm::vec3 normal{};
        glm::vec2 uv{};
        glm::vec4 tangent{};

        static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions() {
            return {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
                {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
                {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
                {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
                {4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
            };
        }

        static std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions() {
            return {
                {
                    .binding = 0,
                    .stride = sizeof(Vertex),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                }
            };
        }
    };


}
