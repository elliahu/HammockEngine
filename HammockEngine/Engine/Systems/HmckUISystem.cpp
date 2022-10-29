#include "HmckUISystem.h"

Hmck::HmckUISystem::HmckUISystem(HmckDevice& device, VkRenderPass renderPass, HmckWindow& window) : hmckDevice{ device }, hmckWindow{ window }, renderPass{renderPass}
{
	init();
}

Hmck::HmckUISystem::~HmckUISystem()
{
	vkDestroyDescriptorPool(hmckDevice.device(), imguiPool, nullptr);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Hmck::HmckUISystem::beginUserInterface()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	//ImGui::SetNextWindowPos({ 10,10 });
	//ImGui::SetNextWindowSize({ 150,50 });
}

void Hmck::HmckUISystem::endUserInterface(VkCommandBuffer commandBuffer)
{
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void Hmck::HmckUISystem::showDebugStats(HmckGameObject& viewerObject)
{
	// get curent cursor position
	double mX, mY;
	glfwGetCursorPos(hmckWindow.getGLFWwindow(), &mX, &mY);

	ImGui::Begin(hmckWindow.getWindowName().c_str());
	ImGui::Text("Camera world POSITION: [ %.2f %.2f %.2f ]", 
		viewerObject.transform.translation.x,
		viewerObject.transform.translation.y,
		viewerObject.transform.translation.z);
	ImGui::Text("Camera world ROTATION: [ %.2f %.2f %.2f ]", 
		viewerObject.transform.rotation.x,
		viewerObject.transform.rotation.y,
		viewerObject.transform.rotation.z);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

void Hmck::HmckUISystem::forward(int button, bool state)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(button, state);
}


void Hmck::HmckUISystem::init()
{
	//1: create descriptor pool for IMGUI
	// the size of the pool is very oversize, but it's copied from imgui demo itself.
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	
	if (vkCreateDescriptorPool(hmckDevice.device(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS)
	{
		std::runtime_error("Failed to create descriptor pool for UI");
	}

	// 2: initialize imgui library

	//this initializes the core structures of imgui
	ImGui::CreateContext();

	// initialize for glfw
	ImGui_ImplGlfw_InitForVulkan(hmckWindow.getGLFWwindow(), true);

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = hmckDevice.getInstance();
	init_info.PhysicalDevice = hmckDevice.getPhysicalDevice();
	init_info.Device = hmckDevice.device();
	init_info.Queue = hmckDevice.graphicsQueue();
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info, renderPass);

	//execute a gpu command to upload imgui font textures
	VkCommandBuffer command_buffer = beginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
	endSingleTimeCommands(command_buffer);

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	//add then destroy the imgui created structures
	// done in the destructor
}

VkCommandBuffer Hmck::HmckUISystem::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = hmckDevice.getCommandPool();
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(hmckDevice.device(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Hmck::HmckUISystem::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(hmckDevice.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(hmckDevice.graphicsQueue());

	vkFreeCommandBuffers(hmckDevice.device(), hmckDevice.getCommandPool(), 1, &commandBuffer);
}

