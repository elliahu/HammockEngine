#pragma once
#include <bitset>
#include <cstring>
#include <memory>
#include <tiny_gltf.h>
#include <unordered_map>
#include <variant>

#include <vulkan/vulkan.h>

#include "hammock/core/Resource.h"
#include "hammock/core/Image.h"
#include "hammock/core/Buffer.h"

namespace hammock {
    namespace rendergraph {

        // This class acts as a wrapper of resource
        // it holds its handle as a reference to the resource
        template<typename T>
        class RenderGraphResourceNode {
        public:
            explicit RenderGraphResourceNode(const ResourceType type,
                                             const std::string &debug_name,
                                             ResourceHandle<T> handle): handle(handle), type(type) {
                static uint64_t next_nid = 0;
                nid = next_nid++;
                this->debug_name = debug_name;
            };

            const ResourceType type;
            std::string debug_name;

            ResourceHandle<T> handle;

            [[nodiscard]] uint64_t getNid() const { return nid; }


            std::vector<uint64_t> isWrittenTo{};
            std::vector<uint64_t> isReadFrom{};

        private:
            uint64_t nid;
        };

        enum class RenderPassType {
            Graphics, Compute, Transfer
        };

        class RenderPass {
        public:
            RenderPassType type;

            std::string debugName;

            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<Attachment>, ResourceAccess> >
            colorAttachmentsAccesses;
            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<Attachment>, ResourceAccess> >
            depthStencilAttachmentsAccesses;
            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<SampledImage>, ResourceAccess> >
            texturesAccesses;
            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<Buffer>, ResourceAccess> > bufferAccesses;

            std::vector<uint64_t> readsFrom;
            std::vector<uint64_t> writesTo;

            RenderPass(const std::string &name, RenderPassType passType): debugName(name), type(passType) {
                static uint64_t next_uid = 0;
                uid = next_uid++;
            }

            // Builder methods

            RenderPass &writeColorAttachment(RenderGraphResourceNode<Attachment> &attachment, ResourceAccess access) {
                colorAttachmentsAccesses.emplace(attachment.getNid(), std::make_pair(attachment, access));
                writesTo.push_back(attachment.getNid());
                attachment.isWrittenTo.push_back(getUid());
                return *this;
            }

            RenderPass &writeDepthStencilAttachment(RenderGraphResourceNode<Attachment> &depthStencil,
                                                    ResourceAccess access) {
                depthStencilAttachmentsAccesses.emplace(depthStencil.getNid(), std::make_pair(depthStencil, access));
                writesTo.push_back(depthStencil.getNid());
                depthStencil.isWrittenTo.push_back(getUid());
                return *this;
            }

            RenderPass &readColorAttachment(RenderGraphResourceNode<Attachment> &attachment, ResourceAccess access) {
                colorAttachmentsAccesses.emplace(attachment.getNid(), std::make_pair(attachment, access));
                readsFrom.push_back(attachment.getNid());
                attachment.isReadFrom.push_back(getUid());
                return *this;
            }

            RenderPass &readDepthStencilAttachment(RenderGraphResourceNode<Attachment> &depthStencil,
                                                   ResourceAccess access) {
                depthStencilAttachmentsAccesses.emplace(depthStencil.getNid(), std::make_pair(depthStencil, access));
                readsFrom.push_back(depthStencil.getNid());
                depthStencil.isReadFrom.push_back(getUid());
                return *this;
            }

            RenderPass &writeTexture(RenderGraphResourceNode<SampledImage> &texture, ResourceAccess access) {
                texturesAccesses.emplace(texture.getNid(), std::make_pair(texture, access));
                writesTo.push_back(texture.getNid());
                texture.isWrittenTo.push_back(getUid());
                return *this;
            }

            RenderPass &readTexture(RenderGraphResourceNode<SampledImage> &texture, ResourceAccess access) {
                texturesAccesses.emplace(texture.getNid(), std::make_pair(texture, access));
                readsFrom.push_back(texture.getNid());
                texture.isReadFrom.push_back(getUid());
                return *this;
            }

            RenderPass &writeBuffer(RenderGraphResourceNode<Buffer> &buffer, ResourceAccess access) {
                bufferAccesses.emplace(buffer.getNid(), std::make_pair(buffer, access));
                writesTo.push_back(buffer.getNid());
                buffer.isWrittenTo.push_back(getUid());
                return *this;
            }

            RenderPass &readBuffer(RenderGraphResourceNode<Buffer> &buffer, ResourceAccess access) {
                bufferAccesses.emplace(buffer.getNid(), std::make_pair(buffer, access));
                readsFrom.push_back(buffer.getNid());
                buffer.isReadFrom.push_back(getUid());
                return *this;
            }

            std::unique_ptr<RenderPass> build() {


                return std::make_unique<RenderPass>(*this);
            }

            // helper methods
            [[nodiscard]] uid_t getUid() const { return uid; }

        private:
            uint64_t uid;
        };
        ;
    } // namespace rendergraph
} // namespace hammock
