#pragma once
#include <functional>

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
            FragmentShaderRead,
            ComputeShaderRead,
            ComputeShaderWrite,
            RayTracingShaderRead
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
            Image, Buffer
        };

        class Resource {
        public:
            typedef uint64_t uid_t;

            ResourceType type;

            uid_t uid;

            std::string debug_name;
        };


        class ImageResource : public Resource {
        };


        class BufferResource : public Resource {
        };
    }
}
