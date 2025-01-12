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
            Image, // general usage image (any type)
            FramebufferAttachment, // spcialized image used as framebuffer attachment
            SampledImage,   // image along with a sampler
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
            friend struct ResourceFactory;

        protected:
            Device &device;
            uint64_t uid;
            std::string debug_name;
            VkDeviceSize size = 0;
            bool resident; // Whether the resource is currently in GPU memory
            bool lazy; // Just in time loaded (if lazy, resource is loaded to memory when getResource is called,
            // if !lazy, resource is loaded into memory at creation time)

            Resource(Device &device, uint64_t uid, const std::string &name)
                : uid(uid), debug_name(name), resident(false), lazy(false), device(device) {
            }


        public:
            virtual ~Resource() = default;

            virtual void load() = 0;

            virtual void unload() = 0;

            uint64_t getUid() const { return uid; }
            const std::string &getName() const { return debug_name; }
            bool isResident() const { return resident; }
            bool isLazy() const {return lazy;}
            VkDeviceSize getSize() const { return size; } // for now
        };


        class ResourceFactory final {
            friend class ResourceManager;
            template<typename T, typename... Args>
            static std::unique_ptr<T> create( Device &device, uint64_t id, const std::string &name, Args &&... args) {
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
            explicit ResourceManager(Device &device, VkDeviceSize memoryBudget = 6ULL * 1024 * 1024 * 1024)
            // 6GB default
                : device(device), totalMemoryUsed(0), memoryBudget(memoryBudget), nextId(1) {
            }

            ~ResourceManager();

            template<typename T, typename... Args>
            ResourceHandle<T> createResource(const std::string &name, Args &&... args) {
                auto resource = ResourceFactory::create<T>(device, nextId, name, std::forward<Args>(args)...);
                uint64_t id = nextId++;

                if (totalMemoryUsed + resource->getSize() > memoryBudget) {
                    evictResources(resource->getSize());
                }

                if (!resource->isLazy()) {
                    resource->load();
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

            void releaseResource(uint64_t id);

        private:
            uint64_t getCurrentTimestamp();

            void evictResources(VkDeviceSize requiredSize);
        };
    }
}
