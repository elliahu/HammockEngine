#include "HmckUserInterface.h"

Hmck::UserInterface::UserInterface(Device& device, VkRenderPass renderPass, Window& window) : hmckDevice{ device }, hmckWindow{ window }, renderPass{renderPass}
{
	init();
	setupStyle();
}

Hmck::UserInterface::~UserInterface()
{
	vkDestroyDescriptorPool(hmckDevice.device(), imguiPool, nullptr);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Hmck::UserInterface::beginUserInterface()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Hmck::UserInterface::endUserInterface(VkCommandBuffer commandBuffer)
{
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void Hmck::UserInterface::showDebugStats(GameObject& camera)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	ImGui::SetNextWindowPos({10,10});
	ImGui::SetNextWindowBgAlpha(0.35f);
	ImGui::Begin(hmckWindow.getWindowName().c_str(),(bool*)0, window_flags);
	ImGui::Text("Camera world position: ( %.2f, %.2f, %.2f )", 
		camera.transformComponent.translation.x,
		camera.transformComponent.translation.y,
		camera.transformComponent.translation.z);
	ImGui::Text("Camera world rotaion: ( %.2f, %.2f, %.2f )", 
		camera.transformComponent.rotation.x,
		camera.transformComponent.rotation.y,
		camera.transformComponent.rotation.z);
	if (ImGui::IsMousePosValid())
		ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
	else
		ImGui::Text("Mouse Position: <invalid or hidden>");
	ImGui::Text("Window resolution: (%d x %d)", hmckWindow.getExtent().width, hmckWindow.getExtent().height);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

void Hmck::UserInterface::showWindowControls()
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	ImGui::SetNextWindowPos({ static_cast<float>(hmckWindow.getExtent().width - 10),10.f}, ImGuiCond_Always, {1.f, 0.f});
	ImGui::SetNextWindowBgAlpha(0.35f);
	ImGui::Begin("Window controls", (bool*)0, window_flags);
	if (ImGui::TreeNode("Window mode"))
	{
		if (ImGui::Button("Fullscreen"))
		{
			hmckWindow.setWindowMode(HMCK_WINDOW_MODE_FULLSCREEN);
		}
		if (ImGui::Button("Borderless"))
		{
			hmckWindow.setWindowMode(HMCK_WINDOW_MODE_BORDERLESS);
		}
		if (ImGui::Button("Windowed"))
		{
			hmckWindow.setWindowMode(HMCK_WINDOW_MODE_WINDOWED);
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Resolution"))
	{
		static int x = hmckWindow.getExtent().width, y = hmckWindow.getExtent().height;
		ImGui::DragInt("Horizontal", &x, 1.f, 800, 3840);
		ImGui::DragInt("Vertical", &y, 1.f, 600, 2160);
		if (ImGui::Button("Apply"))
		{
			hmckWindow.setWindowResolution(x, y);
		}
		ImGui::TreePop();
	}
	ImGui::End();
}

void Hmck::UserInterface::showGameObjectComponents(GameObject& gameObject, bool* close)
{
	beginWindow(gameObject.getName().c_str(), close, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Components attached to GameObject");
	gameObjectComponets(gameObject);
	endWindow();
}

void Hmck::UserInterface::showGameObjectsInspector(GameObject::Map& gameObjects)
{
	const ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::SetNextWindowPos({ 10, 130}, ImGuiCond_Once, {0,0});
	ImGui::SetNextWindowSizeConstraints({ 300, 200 }, ImVec2(static_cast<float>(hmckWindow.getExtent().width), 500));
	beginWindow("GameObjects Inspector", (bool*)false, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Inspect all GameObjects in the scene", window_flags);

	for (auto& kv : gameObjects)
	{
		GameObject& gameObject = kv.second;

		if (ImGui::TreeNode(gameObject.getName().c_str()))
		{
			gameObjectComponets(gameObject);
			ImGui::TreePop();
		}
		ImGui::Separator();
	}
	endWindow();
}

void Hmck::UserInterface::showGlobalSettings(GlobalUbo& ubo)
{
	const ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::SetNextWindowPos({ 10, 800 }, ImGuiCond_Once, { 0,0 });
	ImGui::SetNextWindowSizeConstraints({ 300, 200 }, ImVec2(static_cast<float>(hmckWindow.getExtent().width), 500));
	beginWindow("Global UBO settings", (bool*)false, ImGuiWindowFlags_AlwaysAutoResize);
	if (ImGui::TreeNode("Ambient"))
	{
		float* target[3] = {
				&ubo.ambientLightColor.x,
				&ubo.ambientLightColor.y,
				&ubo.ambientLightColor.z
		};
		ImGui::ColorEdit3("Color", target[0], ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB);
		ImGui::DragFloat("Ambient strength",&ubo.ambientLightColor.w, 0.001f, 0.0f, 1.0f);
		ImGui::TreePop();
	}
	ImGui::Separator();
	endWindow();
}

void Hmck::UserInterface::showLog()
{
	// TODO
}

void Hmck::UserInterface::forward(int button, bool state)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(button, state);
}


void Hmck::UserInterface::init()
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
	pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
	pool_info.pPoolSizes = pool_sizes;

	
	if (vkCreateDescriptorPool(hmckDevice.device(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool for UI");
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

void Hmck::UserInterface::setupStyle()
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

VkCommandBuffer Hmck::UserInterface::beginSingleTimeCommands()
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

void Hmck::UserInterface::endSingleTimeCommands(VkCommandBuffer commandBuffer)
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

void Hmck::UserInterface::beginWindow(const char* title, bool* open, ImGuiWindowFlags flags)
{
	ImGui::Begin(title, open, flags);
}

void Hmck::UserInterface::endWindow()
{
	ImGui::End();
}

void Hmck::UserInterface::gameObjectComponets(GameObject& gameObject)
{
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) // Tranform
	{
		if (ImGui::TreeNode("Translation"))
		{
			ImGui::DragFloat("x", &gameObject.transformComponent.translation.x, 0.01f);
			ImGui::DragFloat("y", &gameObject.transformComponent.translation.y, 0.01f);
			ImGui::DragFloat("z", &gameObject.transformComponent.translation.z, 0.01f);
			ImGui::TreePop();
		}
		//ImGui::Separator();
		if (ImGui::TreeNode("Rotation"))
		{
			ImGui::DragFloat("x", &gameObject.transformComponent.rotation.x, 0.01f);
			ImGui::DragFloat("y", &gameObject.transformComponent.rotation.y, 0.01f);
			ImGui::DragFloat("z", &gameObject.transformComponent.rotation.z, 0.01f);
			ImGui::TreePop();
		}
		//ImGui::Separator();
		if (ImGui::TreeNode("Scale"))
		{
			ImGui::DragFloat("x", &gameObject.transformComponent.scale.x, 0.01f);
			ImGui::DragFloat("y", &gameObject.transformComponent.scale.y, 0.01f);
			ImGui::DragFloat("z", &gameObject.transformComponent.scale.z, 0.01f);
			ImGui::TreePop();
		}
	}
	if (ImGui::CollapsingHeader("Color")) // Color
	{
		float* color_hsv[3] = {
			&gameObject.colorComponent.x,
			&gameObject.colorComponent.y,
			&gameObject.colorComponent.z,
		};
		ImGui::ColorEdit3("Color", color_hsv[0], ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB);
	}
	if (gameObject.boundingBoxComponent != nullptr) // Bounding box
	{
		if (ImGui::CollapsingHeader("Bounding box"))
		{
			ImGui::DragFloat("x min", &gameObject.boundingBoxComponent->x.min, 0.01f);
			ImGui::DragFloat("x max", &gameObject.boundingBoxComponent->x.max, 0.01f);
			ImGui::DragFloat("y min", &gameObject.boundingBoxComponent->y.min, 0.01f);
			ImGui::DragFloat("y max", &gameObject.boundingBoxComponent->y.max, 0.01f);
			ImGui::DragFloat("z min", &gameObject.boundingBoxComponent->z.min, 0.01f);
			ImGui::DragFloat("z max", &gameObject.boundingBoxComponent->z.max, 0.01f);
		}
	}
	if (gameObject.pointLightComponent != nullptr) // Point light
	{
		if (ImGui::CollapsingHeader("Point light"))
		{
			ImGui::DragFloat("Intensity", &gameObject.pointLightComponent->lightIntensity, 0.01f, 0.0f);
			ImGui::DragFloat("Quadratic Term", &gameObject.pointLightComponent->quadraticTerm, 0.01f, 0.0f, 4.0f);
			ImGui::DragFloat("Linear Term", &gameObject.pointLightComponent->linearTerm, 0.01f, 0.0f, 4.0f);
			ImGui::DragFloat("Constant Term", &gameObject.pointLightComponent->constantTerm, 0.01f, 0.0f, 4.0f);
		}
	}
	if (gameObject.directionalLightComponent != nullptr) // directional light
	{
		if (ImGui::CollapsingHeader("Directional light"))
		{
			ImGui::DragFloat("Intensity", &gameObject.directionalLightComponent->lightIntensity, 0.01f, 0.0f);
			ImGui::DragFloat("FOV", &gameObject.directionalLightComponent->fov, 1.0f, 10.0f, 180.f);
			ImGui::DragFloat("Near", &gameObject.directionalLightComponent->_near, 0.01f, 0.001f);
			ImGui::DragFloat("Far", &gameObject.directionalLightComponent->_far, 1.0f, 0.001f);
			float* target[3] = {
				&gameObject.directionalLightComponent->target.x,
				&gameObject.directionalLightComponent->target.y,
				&gameObject.directionalLightComponent->target.z
			};
			ImGui::DragFloat3("Target", target[0],0.1f);
		}
	}
}


