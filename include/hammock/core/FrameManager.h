#pragma once

#include <memory>
#include <vector>
#include <cassert>

#include "hammock/platform/Window.h"
#include "hammock/core/Device.h"
#include "hammock/core/SwapChain.h"
#include "hammock/core/Framebuffer.h"


namespace hammock {
    class FrameManager {
    public:
        FrameManager(Window &window, Device &device);

        ~FrameManager();

        // delete copy constructor and copy destructor
        FrameManager(const FrameManager &) = delete;

        FrameManager &operator=(const FrameManager &) = delete;

        [[nodiscard]] SwapChain* getSwapChain() const {return swapChain.get();};
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

        [[nodiscard]] int getImageIndex() const {
          assert(isFrameStarted && "Cannot get image index when frame not in progress");
            return currentImageIndex;
        }

        VkCommandBuffer beginFrame();
        void endFrame();

    private:
        void createCommandBuffers();

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
