#pragma once

#include "Platform/HmckWindow.h"
#include "HmckDevice.h"
#include "HmckGameObject.h"
#include "HmckRenderer.h"
#include "Systems/HmckDeferredRenderSystem.h"
#include "Systems/HmckUISystem.h"
#include "HmckCamera.h"
#include "Controllers/KeyboardMovementController.h"
#include "HmckBuffer.h"
#include "HmckDescriptors.h"
#include "HmckCollisionDetectionSystem.h"
#include "Utils/HmckLogger.h"
#include "HmckGLTF.h"
#include "HmckBindless.h"

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
	class App
	{
	public:
		static constexpr int WINDOW_WIDTH = 1920;
		static constexpr int WINDOW_HEIGHT = 1080;

		App();

		// delete copy constructor and copy destructor
		App(const App&) = delete;
		App& operator=(const App&) = delete;

		void run();

	private:
		void loadGameObjects();

		HmckWindow hmckWindow{ WINDOW_WIDTH, WINDOW_HEIGHT, "Hammock Engine" };
		HmckDevice hmckDevice{ hmckWindow }; // TODO this is wrong, device should not be dependent on window -> should be otherway around
		HmckRenderer hmckRenderer{ hmckWindow, hmckDevice };

		std::unique_ptr<HmckDescriptorPool> globalPool{};
		std::unique_ptr<HmckDescriptorSetLayout> globalSetLayout{};
		std::unique_ptr<HmckDescriptorSetLayout> materialLayout{};
		HmckGameObject::Map gameObjects;
	};

}
