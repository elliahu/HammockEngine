#pragma once

#include "../HmckPipeline.h"
#include "../HmckDevice.h"
#include "../HmckGameObject.h"
#include "../HmckCamera.h"
#include "../HmckFrameInfo.h"
#include "../HmckDescriptors.h"
#include "../HmckSwapChain.h"
#include "../HmckMesh.h"
#include "HmckIRenderSystem.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <memory>
#include <vector>
#include <stdexcept>

#ifndef SHADERS_DIR
	#define SHADERS_DIR "../../HammockEngine/Engine/Shaders/"
#endif



/*
 *	A system is a process which acts on all GameObjects with the desired components.
 */

/*
	At this point app functions as a Master Render System
	and this class function as subsystem

	subject to change in near future
*/

namespace Hmck
{
	class HmckDeferredRenderSystem: public HmckIRenderSystem
	{
	public:
		HmckDeferredRenderSystem(HmckDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& setLayouts);
		~HmckDeferredRenderSystem();

		// delete copy constructor and copy destructor
		HmckDeferredRenderSystem(const HmckDeferredRenderSystem&) = delete;
		HmckDeferredRenderSystem& operator=(const HmckDeferredRenderSystem&) = delete;

		VkDescriptorSet getShadowmapDescriptorSet() { return shadowmapDescriptorSet; }
		VkDescriptorSetLayout getShadowmapDescriptorSetLayout() { return shadowmapDescriptorLayout->getDescriptorSetLayout(); }

		void updateShadowmapDescriptorSet(VkDescriptorImageInfo& imageInfo);
		void updateGbufferDescriptorSet(std::array<VkDescriptorImageInfo, 4> imageInfos);


		void render(HmckFrameInfo& frameInfo);


	private:
		void createPipelineLayout(std::vector<VkDescriptorSetLayout>& setLayouts);
		void createPipeline(VkRenderPass renderPass);
		void prepareDescriptors();

		// descriptors
		std::unique_ptr<HmckDescriptorPool> descriptorPool{};
		std::unique_ptr<HmckDescriptorSetLayout> shadowmapDescriptorLayout{};
		std::unique_ptr<HmckDescriptorSetLayout> gbufferDescriptorLayout{};
		VkDescriptorSet shadowmapDescriptorSet;
		VkDescriptorSet gbufferDescriptorSet;
		
		
	};

}
