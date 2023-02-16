#pragma once

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <string>
#include <vector>
#include <memory>

#include "HmckDevice.h"
#include "HmckUtils.h"

#define FB_DIM 512
#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

namespace Hmck {

    class HmckSwapChain {

        struct FrameBufferAttachment {
            VkImage image;
            VkDeviceMemory mem;
            VkImageView view;
        };

        struct OffscreenRenderPass {
            int32_t width, height;
            VkFramebuffer frameBuffer;
            FrameBufferAttachment depth;
            VkRenderPass renderPass;
            VkSampler sampler;
            VkDescriptorImageInfo descriptor;
        };

    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        HmckSwapChain(HmckDevice& deviceRef, VkExtent2D windowExtent);
        HmckSwapChain(HmckDevice& deviceRef, VkExtent2D windowExtent, std::shared_ptr<HmckSwapChain> previous);
        ~HmckSwapChain();

        HmckSwapChain(const HmckSwapChain&) = delete;
        HmckSwapChain& operator=(const HmckSwapChain&) = delete;

        VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
        VkFramebuffer getOffscreenFrameBuffer() { return offscreenRenderPass.frameBuffer; }
        VkRenderPass getRenderPass() { return renderPass; }
        VkRenderPass getOffscreenRenderPass() { return offscreenRenderPass.renderPass; }
        VkDescriptorImageInfo getOffscreenDescriptorImageInfo() { return offscreenRenderPass.descriptor; }
        VkImageView getImageView(int index) { return swapChainImageViews[index]; }
        size_t imageCount() { return swapChainImages.size(); }
        VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
        VkExtent2D getSwapChainExtent() { return swapChainExtent; }
        uint32_t width() { return swapChainExtent.width; }
        uint32_t height() { return swapChainExtent.height; }

        float extentAspectRatio() {
            return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        }
        VkFormat findDepthFormat();

        VkResult acquireNextImage(uint32_t* imageIndex);
        VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

        bool compareSwapFormats(const HmckSwapChain& swapChain) const 
        {
            return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
                swapChain.swapChainImageFormat == swapChainImageFormat;
        }

    private:
        void init();
        void createSwapChain();
        void createImageViews();
        void createDepthResources();
        void createRenderPass();
        void createOffscreenRenderPass();
        void createFramebuffers();
        void createSyncObjects();

        // Helper functions
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(
            const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        VkFormat swapChainImageFormat;
        VkFormat swapChainDepthFormat;
        VkExtent2D swapChainExtent;

        std::vector<VkFramebuffer> swapChainFramebuffers;
        VkRenderPass renderPass;

        OffscreenRenderPass offscreenRenderPass;

        std::vector<VkImage> depthImages;
        std::vector<VkDeviceMemory> depthImageMemorys;
        std::vector<VkImageView> depthImageViews;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;


        HmckDevice& device;
        VkExtent2D windowExtent;

        VkSwapchainKHR swapChain;
        std::shared_ptr<HmckSwapChain> oldSwapChain;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        size_t currentFrame = 0;
    };

}  // namespace Hmck