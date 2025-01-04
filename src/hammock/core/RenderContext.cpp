#include "hammock/core/RenderContext.h"

Hammock::RenderContext::RenderContext(Window &window, Device &device) : window{window}, device{device} {
    recreateSwapChain();
    createCommandBuffer();
}

Hammock::RenderContext::~RenderContext() {
    freeCommandBuffers();
}


void Hammock::RenderContext::createCommandBuffer() {
    commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer");
    }
}


void Hammock::RenderContext::freeCommandBuffers() {
    vkFreeCommandBuffers(
        device.device(),
        device.getCommandPool(),
        static_cast<uint32_t>(commandBuffers.size()),
        commandBuffers.data());
    commandBuffers.clear();
}

void Hammock::RenderContext::recreateSwapChain() {
    auto extent = window.getExtent();

    while (extent.width == 0 || extent.height == 0) {
        window.pollEvents();
        extent = window.getExtent();
    }
    vkDeviceWaitIdle(device.device());

    if (hmckSwapChain == nullptr) {
        hmckSwapChain = std::make_unique<SwapChain>(device, extent);
    } else {
        std::shared_ptr<SwapChain> oldSwapChain = std::move(hmckSwapChain);
        hmckSwapChain = std::make_unique<SwapChain>(device, extent, oldSwapChain);

        if (!oldSwapChain->compareSwapFormats(*hmckSwapChain.get())) {
            throw std::runtime_error("Swapchain image (or detph) format has changed");
        }
    }
}

VkCommandBuffer Hammock::RenderContext::beginFrame() {
    assert(!isFrameStarted && "Cannot call beginFrame while already in progress");

    auto result = hmckSwapChain->acquireNextImage(&currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to aquire swap chain image!");
    }

    isFrameStarted = true;

    auto commandBuffer = getCurrentCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording commadn buffer");
    }
    return commandBuffer;
}

void Hammock::RenderContext::endFrame() {
    assert(isFrameStarted && "Cannot call endFrame while frame is not in progress");
    const auto commandBuffer = getCurrentCommandBuffer();

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }

    auto result = hmckSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
        // Window was resized (resolution was changed)
        window.resetWindowResizedFlag();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }

    isFrameStarted = false;

    currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}


void Hammock::RenderContext::beginRenderPass(
    const std::unique_ptr<Framebuffer> &framebuffer,
    const VkCommandBuffer commandBuffer,
    const std::vector<VkClearValue> &clearValues) const {
    assert(isFrameInProgress() && "Cannot call beginRenderPass if frame is not in progress");
    assert(
        commandBuffer == getCurrentCommandBuffer() &&
        "Cannot begin render pass on command buffer from a different frame");
    assert(framebuffer->renderPass && "Provided framebuffer doesn't have a render pass");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = framebuffer->renderPass;
    renderPassInfo.framebuffer = framebuffer->framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {framebuffer->width, framebuffer->height};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = Hammock::Init::viewport(
        static_cast<float>(framebuffer->width),
        static_cast<float>(framebuffer->height),
        0.0f, 1.0f);
    VkRect2D scissor{{0, 0}, {framebuffer->width, framebuffer->height}};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}


void Hammock::RenderContext::beginSwapChainRenderPass(const VkCommandBuffer commandBuffer) const {
    assert(isFrameInProgress() && "Cannot call beginSwapChainRenderPass if frame is not in progress");
    assert(
        commandBuffer == getCurrentCommandBuffer() &&
        "Cannot begin render pass on command buffer from a different frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = hmckSwapChain->getRenderPass();
    renderPassInfo.framebuffer = hmckSwapChain->getFrameBuffer(currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = hmckSwapChain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = HMCK_CLEAR_COLOR; // clear color
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = Hammock::Init::viewport(
        static_cast<float>(hmckSwapChain->getSwapChainExtent().width),
        static_cast<float>(hmckSwapChain->getSwapChainExtent().height),
        0.0f, 1.0f);
    VkRect2D scissor{{0, 0}, hmckSwapChain->getSwapChainExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Hammock::RenderContext::endRenderPass(const VkCommandBuffer commandBuffer) const {
    assert(isFrameInProgress() && "Cannot call endActiveRenderPass if frame is not in progress");
    assert(
        commandBuffer == getCurrentCommandBuffer() &&
        "Cannot end render pass on command buffer from a different frame");

    vkCmdEndRenderPass(commandBuffer);
}
