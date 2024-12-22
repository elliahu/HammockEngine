#pragma once

#include <memory>
#include <vector>
#include <cassert>

#include "platform/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckSwapChain.h"
#include "utils/HmckUserInterface.h"
#include "HmckFramebuffer.h"

// black clear color
#define HMCK_CLEAR_COLOR {0.f,0.f,0.f} //{ 0.f,171.f / 255.f,231.f / 255.f,1.f }

#define SHADOW_RES_WIDTH 2048
#define SHADOW_RES_HEIGHT 2048
#define SHADOW_TEX_FILTER VK_FILTER_LINEAR

#define SSAO_RES_MULTIPLIER 1.0

namespace Hmck {
    class RenderContext {
    public:
        RenderContext(Window &window, Device &device);

        ~RenderContext();

        // delete copy constructor and copy destructor
        RenderContext(const RenderContext &) = delete;

        RenderContext &operator=(const RenderContext &) = delete;

        [[nodiscard]] VkRenderPass getSwapChainRenderPass() const { return hmckSwapChain->getRenderPass(); }
        [[nodiscard]] float getAspectRatio() const { return hmckSwapChain->extentAspectRatio(); }
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
        std::unique_ptr<SwapChain> hmckSwapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageIndex;
        int currentFrameIndex{0};
        bool isFrameStarted{false};
    };
}
