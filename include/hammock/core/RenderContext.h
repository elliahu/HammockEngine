#pragma once

#include <memory>
#include <vector>
#include <cassert>

#include "hammock/platform/Window.h"
#include "hammock/core/Device.h"
#include "hammock/core/SwapChain.h"
#include "hammock/core/Framebuffer.h"

// black clear color
#define HMCK_CLEAR_COLOR {0.f,0.f,0.f} //{ 0.f,171.f / 255.f,231.f / 255.f,1.f }

namespace hammock {
    class RenderContext {
    public:
        RenderContext(Window &window, Device &device);

        ~RenderContext();

        // delete copy constructor and copy destructor
        RenderContext(const RenderContext &) = delete;

        RenderContext &operator=(const RenderContext &) = delete;

        [[nodiscard]] SwapChain* getSwapChain() const {return swapChain.get();};
        [[nodiscard]] VkRenderPass getSwapChainRenderPass() const { return swapChain->getRenderPass(); }
        [[nodiscard]] float getAspectRatio() const { return swapChain->extentAspectRatio(); }
        [[nodiscard]] bool isFrameInProgress() const { return isFrameStarted; }


        [[nodiscard]] VkCommandBuffer getCurrentCommandBuffer() const {
            assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
            return commandBuffers[getFrameIndex()];
        }

        [[nodiscard]] int getFrameIndex() const {
            assert(isFrameStarted && "Cannot get frame index when frame not in progress");
            return currentFrameIndex;
        }

        VkCommandBuffer beginFrame();

        void endFrame();

        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer) const;

        void beginRenderPass(
            const std::unique_ptr<Framebuffer> &framebuffer,
            VkCommandBuffer commandBuffer,
            const std::vector<VkClearValue> &clearValues) const;

        void endRenderPass(VkCommandBuffer commandBuffer) const;

    private:
        void createCommandBuffer();

        void freeCommandBuffers();

        void recreateSwapChain();


        Window &window;
        Device &device;
        std::unique_ptr<SwapChain> swapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageIndex;
        int currentFrameIndex{0};
        bool isFrameStarted{false};
    };
}
