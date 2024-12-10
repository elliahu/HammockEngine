#pragma once

#include "io/HmckWindow.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

// std lib headers
#include <string>

#include "HmckVulkanInstance.h"

namespace Hmck {
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;
        [[nodiscard]] bool isComplete() const { return graphicsFamilyHasValue && presentFamilyHasValue; }
    };

    class Device {
    public:
        Device(VulkanInstance &instance, VkSurfaceKHR surface);

        ~Device();

        // Not copyable or movable
        Device(const Device &) = delete;

        Device &operator=(const Device &) = delete;

        Device(Device &&) = delete;

        Device &operator=(Device &&) = delete;

        [[nodiscard]] VkCommandPool getCommandPool() const { return commandPool; }
        [[nodiscard]] VkDevice device() const { return device_; }
        [[nodiscard]] VkInstance getInstance() const { return instance.getInstance(); }
        [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        [[nodiscard]] VkSurfaceKHR surface() const { return surface_; }
        [[nodiscard]] VkQueue graphicsQueue() const { return graphicsQueue_; }
        [[nodiscard]] VkQueue presentQueue() const { return presentQueue_; }

        [[nodiscard]] SwapChainSupportDetails getSwapChainSupport() const {
            return querySwapChainSupport(physicalDevice);
        }

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

        [[nodiscard]] QueueFamilyIndices findPhysicalQueueFamilies() const { return findQueueFamilies(physicalDevice); }

        VkFormat findSupportedFormat(
            const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

        // Buffer Helper Functions
        void createBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer &buffer,
            VkDeviceMemory &bufferMemory) const;

        VkCommandBuffer beginSingleTimeCommands() const;

        void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;

        void copyBufferToImage(
            VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount,
            uint32_t baseArrayLayer = 0, uint32_t depth = 1) const;

        // Image helper functionss
        void createImageWithInfo(
            const VkImageCreateInfo &imageInfo,
            VkMemoryPropertyFlags properties,
            VkImage &image,
            VkDeviceMemory &imageMemory) const;

        void transitionImageLayout(
            VkImage image,
            VkImageLayout layoutOld,
            VkImageLayout layoutNew,
            uint32_t layerCount = 1,
            uint32_t baseLayer = 0,
            uint32_t levelCount = 1,
            uint32_t baseLevel = 0
        ) const;

        void copyImageToHostVisibleImage(VkImage srcImage, VkImage dstImage,
                                         uint32_t width, uint32_t height) const;


        VkPhysicalDeviceProperties properties;

    private:
        // TODO create instance

        void setupDebugMessenger();

        void pickPhysicalDevice();

        void createLogicalDevice();

        void createCommandPool();

        // helper functions
        bool isDeviceSuitable(VkPhysicalDevice device);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const; // <- surface needed here


        bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const; // <- surface needed here

        VulkanInstance &instance;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkCommandPool commandPool;

        VkDevice device_;
        VkSurfaceKHR surface_; // <- window is needed for this
        VkQueue graphicsQueue_;
        VkQueue presentQueue_;

        const std::vector<const char *> deviceExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            // VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            // VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            // VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            // VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            // VK_KHR_SPIRV_1_4_EXTENSION_NAME,
            // VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
            // VK_KHR_RAY_QUERY_EXTENSION_NAME
        };
    };
}
