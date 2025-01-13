#pragma once

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <string>
#include <vector>
#include <memory>

#include "hammock/core/Device.h"


#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

namespace hammock {
    class SwapChain {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        SwapChain(Device &deviceRef, VkExtent2D windowExtent);

        SwapChain(Device &deviceRef, VkExtent2D windowExtent, const std::shared_ptr<SwapChain> &previous);

        ~SwapChain();

        SwapChain(const SwapChain &) = delete;

        SwapChain &operator=(const SwapChain &) = delete;

        [[nodiscard]] VkFramebuffer getFrameBuffer(const int index) const { return swapChainFramebuffers[index]; }
        [[nodiscard]] VkRenderPass getRenderPass() const { return renderPass; }
        [[nodiscard]] VkImage getImage(const int index) const { return swapChainImages[index]; }
        [[nodiscard]] VkImageView getImageView(const int index) const { return swapChainImageViews[index]; }
        [[nodiscard]] size_t imageCount() const { return swapChainImages.size(); }
        [[nodiscard]] VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
        [[nodiscard]] VkExtent2D getSwapChainExtent() const { return swapChainExtent; }
        [[nodiscard]] uint32_t width() const { return swapChainExtent.width; }
        [[nodiscard]] uint32_t height() const { return swapChainExtent.height; }

        [[nodiscard]] float extentAspectRatio() const {
            return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
        }

        VkFormat findDepthFormat() const;

        VkResult acquireNextImage(uint32_t *imageIndex) const;

        VkResult submitCommandBuffers(const VkCommandBuffer *buffers, const uint32_t *imageIndex);

        [[nodiscard]] bool compareSwapFormats(const SwapChain &swapChain) const {
            return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
                   swapChain.swapChainImageFormat == swapChainImageFormat;
        }

    private:
        void init();

        void createSwapChain();

        void createImageViews();

        void createDepthResources();

        void createRenderPass();

        void createFramebuffers();

        void createSyncObjects();

        // Helper functions
        static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR> &availableFormats);

        static VkPresentModeKHR chooseSwapPresentMode(
            const std::vector<VkPresentModeKHR> &availablePresentModes);

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;

        VkFormat swapChainImageFormat;
        VkFormat swapChainDepthFormat;
        VkExtent2D swapChainExtent;

        std::vector<VkFramebuffer> swapChainFramebuffers;
        VkRenderPass renderPass;


        std::vector<VkImage> depthImages;
        std::vector<VkDeviceMemory> depthImageMemorys;
        std::vector<VkImageView> depthImageViews;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;


        Device &device;
        VkExtent2D windowExtent;

        VkSwapchainKHR swapChain;
        std::shared_ptr<SwapChain> oldSwapChain;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        size_t currentFrame = 0;
    };
} // namespace Hmck
