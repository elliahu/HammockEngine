#include "HmckUISystem.h"

Hmck::HmckUISystem::HmckUISystem(HmckDevice& device, VkRenderPass renderPass, HmckWindow& window) : hmckDevice{ device }, hmckWindow{ window }, renderPass{renderPass}
{
	init();
	setupStyle();
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

	if(showDemoWindow)
		ImGui::ShowDemoWindow();
}

void Hmck::HmckUISystem::endUserInterface(VkCommandBuffer commandBuffer)
{
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void Hmck::HmckUISystem::showDebugStats(HmckGameObject& camera)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	ImGui::SetNextWindowPos({10,10});
	ImGui::SetNextWindowBgAlpha(0.35f);
	ImGui::Begin(hmckWindow.getWindowName().c_str(),(bool*)0, window_flags);
	ImGui::Text("Camera world position: ( %.2f, %.2f, %.2f )", 
		camera.transform.translation.x,
		camera.transform.translation.y,
		camera.transform.translation.z);
	ImGui::Text("Camera world rotaion: ( %.2f, %.2f, %.2f )", 
		camera.transform.rotation.x,
		camera.transform.rotation.y,
		camera.transform.rotation.z);
	if (ImGui::IsMousePosValid())
		ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
	else
		ImGui::Text("Mouse Position: <invalid or hidden>");
	ImGui::Text("Window resolution: (%d x %d)", hmckWindow.getExtent().width, hmckWindow.getExtent().height);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

void Hmck::HmckUISystem::showGameObjectComponents(HmckGameObject& gameObject, bool* close)
{
	std::string windowTitle = "GameObject id_t " + std::to_string(gameObject.getId());
	beginWindow(windowTitle.c_str(), close, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Components attached to GameObject");
	gameObjectComponets(gameObject);
	endWindow();
}

void Hmck::HmckUISystem::showGameObjectsInspector(HmckGameObject::Map& gameObjects)
{
	beginWindow("GameObjects Inspector", (bool*)false, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Inspect all GameObjects in the scene");

	for (auto& kv : gameObjects)
	{
		HmckGameObject& gameObject = kv.second;

		std::string name = "GameObject id_t " + std::to_string(gameObject.getId());
		if (ImGui::TreeNode(name.c_str()))
		{
			gameObjectComponets(gameObject);
			ImGui::TreePop();
		}
		ImGui::Separator();
	}
	endWindow();
}

void Hmck::HmckUISystem::showLog()
{
	// TODO
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

void Hmck::HmckUISystem::setupStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();

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
	style.WindowPadding = { 10.0f, 10.0f };
	style.FramePadding = { 3.0f, 5.0f };
	style.ItemSpacing = { 6.0f, 6.0f };
	style.ItemInnerSpacing = { 6.0f, 6.0f };
	style.ScrollbarSize = 8.0f;
	style.GrabMinSize = 10.0f;

	// align
	style.WindowTitleAlign = { .5f, .5f };

	// colors
	style.Colors[ImGuiCol_TitleBg] = ImVec4(15 / 255.0f, 15 / 255.0f, 15 / 255.0f, 146 / 240.0f); 
	style.Colors[ImGuiCol_FrameBg] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 255 / 255.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 255 / 255.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(33 / 255.0f, 33 / 255.0f, 33 / 255.0f, 50 / 255.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(200 / 255.0f, 200 / 255.0f, 200 / 255.0f, 255 / 255.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(200 / 255.0f, 200 / 255.0f, 200 / 255.0f, 255 / 255.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(255 / 255.0f, 255 / 255.0f, 255 / 255.0f, 255 / 255.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 79 / 255.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 79 / 255.0f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
	style.Colors[ImGuiCol_Tab] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 79 / 255.0f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(60 / 255.0f, 60 / 255.0f, 60 / 255.0f, 170 / 255.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(80 / 255.0f, 80 / 255.0f, 80 / 255.0f, 170 / 255.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(100 / 255.0f, 100 / 255.0f, 100 / 255.0f, 170 / 255.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(255 / 255.0f, 255 / 255.0f, 255 / 255.0f, 170 / 255.0f);
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

void Hmck::HmckUISystem::beginWindow(const char* title, bool* open, ImGuiWindowFlags flags)
{
	ImGui::Begin(title, open, flags);
}

void Hmck::HmckUISystem::endWindow()
{
	ImGui::End();
}

void Hmck::HmckUISystem::gameObjectComponets(HmckGameObject& gameObject)
{
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) // Tranform
	{
		if (ImGui::TreeNode("Translation"))
		{
			ImGui::DragFloat("x", &gameObject.transform.translation.x, 0.01);
			ImGui::DragFloat("y", &gameObject.transform.translation.y, 0.01);
			ImGui::DragFloat("z", &gameObject.transform.translation.z, 0.01);
			ImGui::TreePop();
		}
		//ImGui::Separator();
		if (ImGui::TreeNode("Rotation"))
		{
			ImGui::DragFloat("x", &gameObject.transform.rotation.x, 0.01);
			ImGui::DragFloat("y", &gameObject.transform.rotation.y, 0.01);
			ImGui::DragFloat("z", &gameObject.transform.rotation.z, 0.01);
			ImGui::TreePop();
		}
		//ImGui::Separator();
		if (ImGui::TreeNode("Scale"))
		{
			ImGui::DragFloat("x", &gameObject.transform.scale.x, 0.01);
			ImGui::DragFloat("y", &gameObject.transform.scale.y, 0.01);
			ImGui::DragFloat("z", &gameObject.transform.scale.z, 0.01);
			ImGui::TreePop();
		}
	}
	if (ImGui::CollapsingHeader("Color")) // Color
	{
		float* color_hsv[3] = {
			&gameObject.color.x,
			&gameObject.color.y,
			&gameObject.color.z,
		};
		ImGui::ColorEdit3("Color", color_hsv[0], ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB);
	}
	if (gameObject.model != nullptr) // Model
	{
		if (ImGui::CollapsingHeader("Model"))
		{
			ImGui::Text("Has model");
		}
	}
	if (gameObject.boundingBox != nullptr) // Bounding box
	{
		if (ImGui::CollapsingHeader("Bounding box"))
		{
			ImGui::DragFloat("x min", &gameObject.boundingBox->x.min, 0.01);
			ImGui::DragFloat("x max", &gameObject.boundingBox->x.max, 0.01);
			ImGui::DragFloat("y min", &gameObject.boundingBox->y.min, 0.01);
			ImGui::DragFloat("y max", &gameObject.boundingBox->y.max, 0.01);
			ImGui::DragFloat("z min", &gameObject.boundingBox->z.min, 0.01);
			ImGui::DragFloat("z max", &gameObject.boundingBox->z.max, 0.01);
		}
	}
	if (gameObject.pointLight != nullptr) // Point light
	{
		if (ImGui::CollapsingHeader("Point light"))
		{
			ImGui::DragFloat("Intensity", &gameObject.pointLight->lightIntensity, 0.01, 0.0, 1.0);
		}
	}
	if (gameObject.directionalLight != nullptr) // directional light
	{
		if (ImGui::CollapsingHeader("Directional light"))
		{
			ImGui::DragFloat("Intensity", &gameObject.directionalLight->lightIntensity, 0.01, 0.0, 1.0);
		}
	}
}


