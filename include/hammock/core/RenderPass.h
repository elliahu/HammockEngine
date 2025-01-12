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
        private:
            void createRenderPass(ResourceManager &manager);

            Device &device;
            uint64_t uid;
            VkRenderPass renderPass;
            VkFramebuffer framebuffer;
            uint32_t width, height;

            RenderPass(Device &device, uint32_t width, uint32_t height, const std::string &name,
                       RenderPassType passType): device(device), width(width), height(height), debugName(name),
                                                 type(passType) {
                static uint64_t next_uid = 0;
                uid = next_uid++;
            }

        public:
            RenderPassType type;

            std::string debugName;

            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<Attachment>, ResourceAccess> >
            colorAttachmentReads;
            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<Attachment>, ResourceAccess> >
            colorAttachmentWrites;
            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<Attachment>, ResourceAccess> >
            depthStencilAttachmentReads;
            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<Attachment>, ResourceAccess> >
            depthStencilAttachmentWrites;
            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<SampledImage>, ResourceAccess> >
            textureReads;
            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<SampledImage>, ResourceAccess> >
            textureWrites;
            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<Buffer>, ResourceAccess> > bufferReads;
            std::unordered_map<uint64_t, std::pair<RenderGraphResourceNode<Buffer>, ResourceAccess> > bufferWrites;



            static RenderPass &create(Device &device, uint32_t width, uint32_t height, const std::string &name,
                              RenderPassType passType) {
                return *(new RenderPass(device, width, height, name, passType));
            }


            ~RenderPass();

            // Builder methods

            RenderPass &writeColorAttachment(RenderGraphResourceNode<Attachment> &attachment, ResourceAccess access) {
                colorAttachmentWrites.emplace(attachment.getNid(), std::make_pair(attachment, access));
                attachment.isWrittenTo.push_back(getUid());
                return *this;
            }

            RenderPass &writeDepthStencilAttachment(RenderGraphResourceNode<Attachment> &depthStencil,
                                                    ResourceAccess access) {
                depthStencilAttachmentWrites.emplace(depthStencil.getNid(), std::make_pair(depthStencil, access));
                depthStencil.isWrittenTo.push_back(getUid());
                return *this;
            }

            RenderPass &readColorAttachment(RenderGraphResourceNode<Attachment> &attachment, ResourceAccess access) {
                colorAttachmentReads.emplace(attachment.getNid(), std::make_pair(attachment, access));
                attachment.isReadFrom.push_back(getUid());
                return *this;
            }

            RenderPass &readDepthStencilAttachment(RenderGraphResourceNode<Attachment> &depthStencil,
                                                   ResourceAccess access) {
                depthStencilAttachmentReads.emplace(depthStencil.getNid(), std::make_pair(depthStencil, access));
                depthStencil.isReadFrom.push_back(getUid());
                return *this;
            }

            RenderPass &writeTexture(RenderGraphResourceNode<SampledImage> &texture, ResourceAccess access) {
                textureWrites.emplace(texture.getNid(), std::make_pair(texture, access));
                texture.isWrittenTo.push_back(getUid());
                return *this;
            }

            RenderPass &readTexture(RenderGraphResourceNode<SampledImage> &texture, ResourceAccess access) {
                textureReads.emplace(texture.getNid(), std::make_pair(texture, access));
                texture.isReadFrom.push_back(getUid());
                return *this;
            }

            RenderPass &writeBuffer(RenderGraphResourceNode<Buffer> &buffer, ResourceAccess access) {
                bufferWrites.emplace(buffer.getNid(), std::make_pair(buffer, access));
                buffer.isWrittenTo.push_back(getUid());
                return *this;
            }

            RenderPass &readBuffer(RenderGraphResourceNode<Buffer> &buffer, ResourceAccess access) {
                bufferReads.emplace(buffer.getNid(), std::make_pair(buffer, access));
                buffer.isReadFrom.push_back(getUid());
                return *this;
            }

            std::unique_ptr<RenderPass> build(ResourceManager &manager) {
                createRenderPass(manager);
                return std::unique_ptr<RenderPass>(this); // Transfer ownership of `this`
            }

            // helper methods
            [[nodiscard]] uid_t getUid() const { return uid; }
            [[nodiscard]] VkRenderPass getRenderPass() const { return renderPass; }
            [[nodiscard]] VkFramebuffer getFramebuffer() const { return framebuffer; }
        };
    } // namespace rendergraph
} // namespace hammock
