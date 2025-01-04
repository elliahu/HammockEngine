#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace Hammock {
    class VulkanInstance {
#ifdef NDEBUG
        const bool enableValidationLayers = false;
#else
        const bool enableValidationLayers = true;
#endif

    public:
        VulkanInstance();

        ~VulkanInstance();

        VkInstance getInstance() const { return instance; };

    private:
        void setupDebugMessenger();

        void createInstance();

        [[nodiscard]] bool checkValidationLayerSupport() const;

        void hasGflwRequiredInstanceExtensions() const;

        [[nodiscard]] std::vector<const char *> getRequiredExtensions() const;

        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    };
}
