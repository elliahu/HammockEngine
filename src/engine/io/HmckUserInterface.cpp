#include "HmckUserInterface.h"
#include "core/HmckGraphicsPipeline.h"
#if defined(_WIN32)
#include "backends/imgui_impl_win32.h"
#endif
#include "backends/imgui_impl_vulkan.h"
#include "utils/HmckUtils.h"
#include <deque>
#include <string>


Hmck::UserInterface::UserInterface(Device &device, const VkRenderPass renderPass, Window &window) : device{device},
    window{window}, renderPass{renderPass} {
    init();
    setupStyle();
}

Hmck::UserInterface::~UserInterface() {
    vkDestroyDescriptorPool(device.device(), imguiPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
#if defined(_WIN32)
    ImGui_ImplWin32_Shutdown();
#endif
    ImGui::DestroyContext();
}

void Hmck::UserInterface::beginUserInterface() {
    ImGui_ImplVulkan_NewFrame();
#if defined(_WIN32)
    ImGui_ImplWin32_NewFrame();
#endif
    ImGui::NewFrame();
}

void Hmck::UserInterface::endUserInterface(VkCommandBuffer commandBuffer) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void Hmck::UserInterface::showDebugStats(const HmckMat4 &inverseView) {
    const ImGuiIO &io = ImGui::GetIO();
    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                              ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                              ImGuiWindowFlags_NoNav;
    ImGui::SetNextWindowPos({10, 10});
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin(window.getWindowName().c_str(), (bool *) nullptr, window_flags);
    const auto cameraPosition = inverseView.Columns[3].XYZ;

    ImGui::Text("Camera world position: ( %.2f, %.2f, %.2f )", cameraPosition.X, cameraPosition.Y, cameraPosition.Z);
    if (ImGui::IsMousePosValid())
        ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
    else
        ImGui::Text("Mouse Position: <invalid or hidden>");
    ImGui::Text("Window resolution: (%d x %d)", window.getExtent().width, window.getExtent().height);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::End();
}


void Hmck::UserInterface::showColorSettings(float *exposure, float *gamma, float *whitePoint) {
    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
    beginWindow("Color settings", (bool *) false, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::DragFloat("Exposure", exposure, 0.01f, 0.01f);
    ImGui::DragFloat("Gamma", gamma, 0.01f, 0.01f);
    ImGui::DragFloat("White point", whitePoint, 0.01f, 0.01f);
    endWindow();
}

void Hmck::UserInterface::forward(const int button, const bool state) {
    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseButtonEvent(button, state);
}


void Hmck::UserInterface::init() {
    //1: create descriptor pool for IMGUI
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    const VkDescriptorPoolSize pool_sizes[] =
    {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;


    if (vkCreateDescriptorPool(device.device(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool for UI");
    }

    // 2: initialize imgui library

    //this initializes the core structures of imgui
    ImGui::CreateContext();

    // initialize for glfw
#if defined(_WIN32)
    ImGui_ImplWin32_Init(window.hWnd);
#endif


    //this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = device.getInstance();
    init_info.PhysicalDevice = device.getPhysicalDevice();
    init_info.Device = device.device();
    init_info.Queue = device.graphicsQueue();
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.RenderPass = renderPass;

    ImGui_ImplVulkan_Init(&init_info);

    //execute a gpu command to upload imgui font textures
    ImGui_ImplVulkan_CreateFontsTexture();

    //clear font textures from cpu data


    //add then destroy the imgui created structures
    // done in the destructor
}

void Hmck::UserInterface::setupStyle() {
    ImGuiStyle &style = ImGui::GetStyle();
    // Setup ImGUI style

    // Rounding
    style.Alpha = .9f;
    float globalRounding = 9.0f;
    style.FrameRounding = globalRounding;
    style.WindowRounding = globalRounding;
    style.ChildRounding = globalRounding;
    style.PopupRounding = globalRounding;
    style.ScrollbarRounding = globalRounding;
    style.GrabRounding = globalRounding;
    style.LogSliderDeadzone = globalRounding;
    style.TabRounding = globalRounding;

    // padding
    style.WindowPadding = {10.0f, 10.0f};
    style.FramePadding = {3.0f, 5.0f};
    style.ItemSpacing = {6.0f, 6.0f};
    style.ItemInnerSpacing = {6.0f, 6.0f};
    style.ScrollbarSize = 8.0f;
    style.GrabMinSize = 10.0f;

    // align
    style.WindowTitleAlign = {.5f, .5f};

    // colors
    style.Colors[ImGuiCol_TitleBg] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(48 / 255.0f, 48 / 255.0f, 54 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(48 / 255.0f, 48 / 255.0f, 54 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(48 / 255.0f, 48 / 255.0f, 54 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(33 / 255.0f, 33 / 255.0f, 33 / 255.0f, 50 / 255.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(200 / 255.0f, 200 / 255.0f, 200 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(200 / 255.0f, 200 / 255.0f, 200 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(255 / 255.0f, 255 / 255.0f, 255 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 79 / 255.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 79 / 255.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(80 / 255.0f, 80 / 255.0f, 80 / 255.0f, 170 / 255.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(241 / 255.0f, 135 / 255.0f, 1 / 255.0f, 255 / 255.0f);
}

VkCommandBuffer Hmck::UserInterface::beginSingleTimeCommands() const {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = device.getCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device.device(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Hmck::UserInterface::endSingleTimeCommands(const VkCommandBuffer commandBuffer) const {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device.graphicsQueue());

    vkFreeCommandBuffers(device.device(), device.getCommandPool(), 1, &commandBuffer);
}

void Hmck::UserInterface::beginWindow(const char *title, bool *open, ImGuiWindowFlags flags) {
    ImGui::Begin(title, open, flags);
}

void Hmck::UserInterface::endWindow() {
    ImGui::End();
}
