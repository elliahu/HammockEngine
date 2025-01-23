#pragma once
#include <functional>
#include <hammock/core/CoreUtils.h>

#include "hammock/core/Image.h"
#include "hammock/core/Buffer.h"
#include "hammock/core/Device.h"

namespace hammock {
    namespace experimental {

        class ResourceManager;
        class ResourceFactory final {
            friend class ResourceManager;

            template<typename T, typename... Args>
            static std::unique_ptr<T> create(Device &device, uint64_t id, Args &&... args) {
                return std::unique_ptr<T>(new T(device, id, std::forward<Args>(args)...));
            }

        };
;

        // Resource Manager class
        class ResourceManager {
        private:
          	using ResourceMap = std::unordered_map<uint64_t, std::unique_ptr<Resource> >;

            Device &device;
            ResourceMap resources;
            std::unordered_map<uint64_t, int32_t> frameDependencies;

            VkDeviceSize totalMemoryUsed;
            VkDeviceSize memoryBudget;
            uint64_t nextId;

            // Cache for frequently used resources
            struct CacheEntry {
                uint64_t lastUsed;
                uint64_t useCount;
            };

            std::unordered_map<uint64_t, CacheEntry> resourceCache;

        public:

            explicit ResourceManager(Device &device, VkDeviceSize memoryBudget = 6ULL * 1024 * 1024 * 1024)
            // 6GB default
                : device(device), totalMemoryUsed(0), memoryBudget(memoryBudget), nextId(1) {
            }

            ~ResourceManager();


            template<int FrameIdx = -1, typename T,  typename... Args>
            ResourceHandle createResource(Args &&... args) {
                static_assert(ResourceTypeTraits<T>::type != ResourceType::Invalid,
                              "Resource type not registered in ResourceTypeTraits");

                auto resource = ResourceFactory::create<T>(device, nextId, std::forward<Args>(args)...);
                uint64_t id = nextId++;

                if (totalMemoryUsed + resource->getSize() > memoryBudget) {
                    evictResources(resource->getSize());
                }

                if (!resource->isLazy()) {
                    resource->load();
                }

                resources[id] = std::move(resource);
                frameDependencies[id] = FrameIdx;
                resourceCache[id] = {getCurrentTimestamp(), 0};

                return ResourceHandle::create(ResourceTypeTraits<T>::type, id);
            }




            template<typename T>
            T *getResource(ResourceHandle handle) {
                // Type check
                if (ResourceTypeTraits<T>::type != handle.getType()) {
                    return nullptr;
                }

                auto it = resources.find(handle.getUid());
                if (it != resources.end()) {
                    auto *resource = static_cast<T *>(it->second.get());

                    // Update cache information
                    resourceCache[handle.getUid()].lastUsed = getCurrentTimestamp();
                    resourceCache[handle.getUid()].useCount++;

                    // Load if not resident
                    if (!resource->isResident()) {
                        resource->load();
                        totalMemoryUsed += resource->getSize();
                    }
                    return resource;
                }
                return nullptr;
            }


            // Helper to get resource type name
            const char *getResourceTypeName(ResourceHandle handle) {
                return handle.getTypeName();
            }

            void releaseResource(uint64_t id);

        private:
            uint64_t getCurrentTimestamp();

            void evictResources(VkDeviceSize requiredSize);
        };
    }
}
