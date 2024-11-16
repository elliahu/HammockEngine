#pragma once
#include <string>
#include <iostream>
#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp> // Include for glm::eulerAngleYXZ function
#include <tiny_gltf.h>
#include "resources/HmckTexture.h"
#include "HmckEntity.h"
#include "HmckEntity3D.h"
#include "core/HmckMemory.h"
#include "HmckLights.h"
#include "HmckScene.h"

namespace gltf = tinygltf;

namespace Hmck {
    class GltfLoader {
    public:
        enum LoadingFlags {
            None = 0x00000000,
            FlipY = 0x00000001,
            PreTransformVertices = 0x00000002
        };

        GltfLoader(Device &device, MemoryManager &memory, std::unique_ptr<Scene> &scene);

        void load(std::string filename, uint32_t fileLoadingFlags = LoadingFlags::None);

    private:
        Device &device;
        MemoryManager &memory;
        std::unique_ptr<Scene> &scene;

        uint32_t imagesOffset;
        uint32_t texturesOffset;
        uint32_t materialsOffset;
        uint32_t loadingFlags = LoadingFlags::None;


        static bool isBinary(const std::string &filename);

        static bool isLight(const gltf::Node &node, gltf::Model &model);

        static bool isSolid(const gltf::Node &node);

        static bool isCamera(const gltf::Node &node);

        void loadImages(gltf::Model &model) const;

        void loadTextures(const gltf::Model &model) const;

        void loadMaterials(const gltf::Model &model) const;

        void loadEntities(gltf::Model &model);

        void loadEntitiesRecursive(gltf::Node &node, gltf::Model &model, const std::shared_ptr<Entity> &parent);

        void loadEntity(const gltf::Node &node, gltf::Model &model, const std::shared_ptr<Entity> &parent) const;

        void loadEntity3D(gltf::Node &node, gltf::Model &model, std::shared_ptr<Entity> parent) const;

        void loadIOmniLight(const gltf::Node &node, gltf::Model &model, const std::shared_ptr<Entity> &parent) const;

        void loadCamera(const gltf::Node &node, const gltf::Model &model, const std::shared_ptr<Entity> &parent) const;
    };
}
