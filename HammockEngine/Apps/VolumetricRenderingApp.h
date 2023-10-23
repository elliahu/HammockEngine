#pragma once
#include "Platform/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckGameObject.h"
#include "HmckRenderer.h"
#include "Systems/HmckUISystem.h"
#include "HmckCamera.h"
#include "Controllers/KeyboardMovementController.h"
#include "HmckBuffer.h"
#include "HmckDescriptors.h"
#include "HmckCollisionDetectionSystem.h"
#include "Utils/HmckLogger.h"
#include "HmckGLTF.h"
#include "IApp.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>
#include <chrono>


#ifndef MODELS_DIR
#define MODELS_DIR "../../Resources/Models/"
#endif // !MODELS_DIR

#ifndef MATERIALS_DIR
#define MATERIALS_DIR "../../Resources/Materials/"
#endif // !MATERIALS_DIR

namespace Hmck 
{
	class VolumetricRenderingApp : public IApp
	{
		// There is so much code in this class 
		// TODO make more abstraction layers over the IApp

	public:

		struct PerFrameData
		{
			glm::mat4 projection{ 1.f };
			glm::mat4 view{ 1.f };
			glm::mat4 inverseView{ 1.f };
		};

		struct PushConstantData
		{
			glm::mat4 model{1.f};
			glm::mat4 normal{1.f};
		};

		VolumetricRenderingApp();

		// Inherited via IApp
		virtual void run() override;
		virtual void load() override;

		std::unique_ptr<HmckDescriptorPool> descriptorPool{};
		std::unique_ptr<HmckDescriptorSetLayout> descriptorSetLayout{};
		std::unique_ptr<HmckDescriptorSetLayout> materialLayout{};

		HmckGameObject::Map gameObjects;
	};

}

