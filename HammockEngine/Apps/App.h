#pragma once

#include "Platform/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckRenderer.h"
#include "Systems/HmckUserInterface.h"
#include "HmckCamera.h"
#include "Controllers/KeyboardMovementController.h"
#include "HmckBuffer.h"
#include "HmckDescriptors.h"
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


/*
namespace Hmck
{
	class App: public IApp
	{
	public:

		App();

		virtual void run() override;

	private:
		virtual void load() override;

		std::unique_ptr<DescriptorPool> globalPool{};
		std::unique_ptr<DescriptorSetLayout> globalSetLayout{};
		std::unique_ptr<DescriptorSetLayout> materialLayout{};
		std::unique_ptr<DescriptorSetLayout> bindlessLayout{};

		GameObject::Map gameObjects;
	};

}
*/