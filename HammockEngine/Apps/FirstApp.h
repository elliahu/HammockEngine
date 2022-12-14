#pragma once

#include "Platforms/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckGameObject.h"
#include "HmckRenderer.h"
#include "Systems/HmckLightSystem.h"
#include "Systems/HmckRenderSystem.h"
#include "Systems/HmckUISystem.h"
#include "HmckModel.h"
#include "HmckCamera.h"
#include "Controllers/KeyboardMovementController.h"
#include "HmckBuffer.h"
#include "HmckDescriptors.h"
#include "HmckCollisionDetectionSystem.h"
#include "Utils/HmckLogger.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>
#include <chrono>

#ifndef MODELS_DIR
#define MODELS_DIR "../../HammockEngine/Models/"
#endif // !MODELS_DIR

#ifndef MATERIALS_DIR
#define MATERIALS_DIR "../../Resources/Materials/"
#endif // !MATERIALS_DIR



namespace Hmck
{
	class FirstApp
	{
	public:
		static constexpr int WINDOW_WIDTH = 1280;
		static constexpr int WINDOW_HEIGHT = 720;

		FirstApp();

		// delete copy constructor and copy destructor
		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		void run();

	private:
		void loadGameObjects();

		HmckWindow hmckWindow{ WINDOW_WIDTH, WINDOW_HEIGHT, "Hammock Engine" };
		HmckDevice hmckDevice{ hmckWindow };
		HmckRenderer hmckRenderer{ hmckWindow, hmckDevice };

		std::unique_ptr<HmckDescriptorPool> globalPool{};
		std::unique_ptr<HmckDescriptorSetLayout> globalSetLayout{};
		std::unique_ptr<HmckDescriptorSetLayout> materialLayout{};
		HmckGameObject::Map gameObjects;
	};

}
