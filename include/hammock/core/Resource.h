#pragma once
#include <functional>
#include <hammock/utils/NonCopyable.h>

#include "DeviceStorage.h"

namespace hammock {
    namespace rendergraph {
        enum class ImageState {
            Undefined,
            General,
            ColorAttachment,
            DepthStencilAttachment,
            ShaderReadOnly,
            TransferSrc,
            TransferDst,
            Present,
        };

        enum class BufferState {
            Undefined,
            TransferSrc,
            TransferDst,
            UniformBuffer,
            StorageBuffer,
            IndexBuffer,
            VertexBuffer,
            IndirectBuffer,
            ShaderBindingTable,
            AccelerationStructureBuild
        };

        enum class ResourceAccessType {
            Read, Write, ReadWrite
        };

        struct ResourceAccess {
            ResourceAccessType type;

            union {
                ImageState imageState;
                BufferState bufferState;
            } requiredState;
        };

        enum class ResourceType {
            ImageView,
            Image,
            FramebufferAttachment,
            Sampler,
            SampledImage,
            Buffer
        };


        template<typename T>
        class ResourceHandle {
            uint64_t uid;

        public:
            ResourceHandle() = default;

            explicit ResourceHandle(uint64_t uid) : uid(uid) {
            }

            uint64_t getUid() const { return uid; }
        };


        class ResourceManager;

        // Base resource class
        class Resource : public NonCopyable {
            friend class ResourceManager;
            friend struct ResourceFactory;

        protected:
            Device &device;
            uint64_t uid;
            std::string debug_name;
            VkDeviceSize size = 0;
            bool resident; // Whether the resource is currently in GPU memory

            Resource(Device &device, uint64_t uid, const std::string &name)
                : uid(uid), debug_name(name), resident(false), device(device) {
            }

        public:
            virtual ~Resource() = default;

            virtual void load() = 0;

            virtual void unload() = 0;

            uint64_t getUid() const { return uid; }
            const std::string &getName() const { return debug_name; }
            bool isLoaded() const { return resident; }
            VkDeviceSize getSize() const { return size; } // for now
        };

        struct ResourceFactory {
            template<typename T, typename... Args>
            static std::unique_ptr<T> create(Device &device, uint64_t id, const std::string &name, Args &&... args) {
                return std::unique_ptr<T>(new T(device, id, name, std::forward<Args>(args)...));
            }
        };


        // Resource Manager class
        class ResourceManager {
        private:
            Device &device;
            std::unordered_map<uint64_t, std::unique_ptr<Resource> > resources;
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
            explicit ResourceManager(Device &device, VkDeviceSize memoryBudget = 6ULL * 1024 * 1024 * 1024) // 6GB default
                : device(device), totalMemoryUsed(0), memoryBudget(memoryBudget), nextId(1) {
            }

            template<typename T, typename... Args>
            ResourceHandle<T> createResource(const std::string &name, Args &&... args) {
                auto resource = ResourceFactory::create<T>(device, nextId, name, std::forward<Args>(args)...);
                uint64_t id = nextId++;

                if (totalMemoryUsed + resource->getSize() > memoryBudget) {
                    throw std::runtime_error("ResourceManager::createResource: Memory budget exceeded");
                    evictResources(resource->getSize());
                }

                resources[id] = std::move(resource);
                resourceCache[id] = {getCurrentTimestamp(), 0};

                return ResourceHandle<T>(id);
            }

            template<typename T>
            T *getResource(ResourceHandle<T> handle) {
                auto it = resources.find(handle.getId());
                if (it != resources.end()) {
                    auto *resource = dynamic_cast<T *>(it->second.get());
                    if (resource) {
                        // Update cache information
                        resourceCache[handle.getId()].lastUsed = getCurrentTimestamp();
                        resourceCache[handle.getId()].useCount++;

                        // Load if not resident
                        if (!resource->isLoaded()) {
                            resource->load();
                            totalMemoryUsed += resource->getSize();
                        }
                        return resource;
                    }
                }
                return nullptr;
            }

            void releaseResource(uint64_t id) {
                auto it = resources.find(id);
                if (it != resources.end()) {
                    if (it->second->isLoaded()) {
                        totalMemoryUsed -= it->second->getSize();
                        it->second->unload();
                    }
                    resources.erase(it);
                    resourceCache.erase(id);
                }
            }

        private:
            uint64_t getCurrentTimestamp() {
                // Implementation to get current time
                return 0; // Placeholder
            }

            void evictResources(VkDeviceSize requiredSize) {
                // Sort resources by last used time and use count
                std::vector<std::pair<uint64_t, CacheEntry> > sortedCache;
                for (const auto &entry: resourceCache) {
                    sortedCache.push_back(entry);
                }

                std::sort(sortedCache.begin(), sortedCache.end(),
                          [](const auto &a, const auto &b) {
                              // Consider both last used time and use frequency
                              return (a.second.lastUsed * a.second.useCount) <
                                     (b.second.lastUsed * b.second.useCount);
                          });

                // Unload resources until we have enough space
                VkDeviceSize freedMemory = 0;
                for (const auto &entry: sortedCache) {
                    auto *resource = resources[entry.first].get();
                    if (resource->isLoaded()) {
                        resource->unload();
                        freedMemory += resource->getSize();
                        totalMemoryUsed -= resource->getSize();

                        if (freedMemory >= requiredSize) {
                            break;
                        }
                    }
                }
            }
        };
    }
}
