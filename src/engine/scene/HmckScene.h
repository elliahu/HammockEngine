#pragma once
#include <string>
#include <memory>

#include "resources/HmckDescriptors.h"
#include "HmckEntity3D.h"
#include "core/HmckSwapChain.h"
#include "HmckCamera.h"
#include "resources/HmckHDR.h"
#include "utils/HmckUtils.h"
#include "HmckVertex.h"

namespace Hmck {
    // TODO add runtime object addition and removal
    class Scene {
    public:
        struct SceneCreateInfo {
            Device &device;
            ResourceManager &memory;
            std::string name;
        };

        // TODO make builder as well
        explicit Scene(const SceneCreateInfo &createInfo);

        ~Scene();

        void destroy() const;

        // delete copy constructor and copy destructor
        Scene(const Scene &) = delete;

        Scene &operator=(const Scene &) = delete;

        void add(const std::shared_ptr<Entity>& entity, const std::shared_ptr<Entity> &parent);


        std::shared_ptr<Entity> getRoot() { return entities[root]; }

        std::shared_ptr<Entity> getEntity(EntityHandle id) {
            if (entities.contains(id)) {
                return entities[id];
            }

            return nullptr;
        }

        Device &getDevice() const { return device; }
        EntityHandle getLastAdded() const { return lastAdded; };
        std::shared_ptr<Camera> getCamera(const EntityHandle handle) { return cast<Entity, Camera>(getEntity(handle)); }
        std::shared_ptr<Camera> getActiveCamera() { return getCamera(activeCamera); }


        void setActiveCamera(const EntityHandle handle) { activeCamera = handle; }

        std::unordered_map<EntityHandle, std::shared_ptr<Entity> > entities{};

        std::vector<Image> images;
        std::vector<Texture> textures;
        std::vector<Material> materials;

        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
        uint32_t vertexCount;
        uint32_t indexCount;

        std::shared_ptr<Environment> environment;

        EntityHandle activeCamera = 0;
        std::vector<EntityHandle> cameras{};
        std::vector<EntityHandle> lights{};

    private:
        Device &device;
        ResourceManager &memory;
        EntityHandle lastAdded = 0;
        EntityHandle root;
    };
}
