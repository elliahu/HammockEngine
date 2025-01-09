#pragma once
#include <bitset>
#include <cstring>
#include <memory>
#include <tiny_gltf.h>
#include <unordered_map>
#include <variant>

#include <vulkan/vulkan.h>

#include "hammock/core/Resource.h"

namespace hammock {
    namespace rendergraph {

        enum class RenderPassType {
            Graphics, Compute, Transfer
        };

        class RenderPass {
        public:
            RenderPassType type;

            std::string debugName;
            std::unordered_map<Resource::uid_t, ResourceAccess> readsFrom;
            std::unordered_map<Resource::uid_t, ResourceAccess> writesTo;

            RenderPass(const std::string& name, RenderPassType passType): debugName(name), type(passType) {}

            RenderPass& addColorTarget(ImageResource attachment, ResourceAccess access) {
                writesTo.emplace(attachment.uid, access);
                return *this;
            }

            RenderPass& addDepthStencilTarget(ImageResource depthStencil, ResourceAccess access) {
                writesTo.emplace(depthStencil.uid, access);
                return *this;
            }

            RenderPass& sampleImage(ImageResource image, ResourceAccess access) {
                readsFrom.emplace(image.uid, access);
                return *this;
            }

            RenderPass& useBuffer(BufferResource buffer, ResourceAccess access) {
                // we currently only read from buffer
                readsFrom.emplace(buffer.uid, access);
                return *this;
            }

            std::unique_ptr<RenderPass> get() {
                return std::make_unique<RenderPass>(*this);
            }

        };
        ;
    } // namespace rendergraph
} // namespace hammock
