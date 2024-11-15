#pragma once
// vulkan headers
#include <vulkan/vulkan.h>
#include "HmckDevice.h"
#include "HmckUtils.h"

namespace Hmck
{
	// Holds information for a ray tracing acceleration structure
	struct AccelerationStructure 
	{
		VkAccelerationStructureKHR handle;
		uint64_t deviceAddress = 0;
		VkDeviceMemory memory;
		VkBuffer buffer;
	};

	// Holds information for a ray tracing scratch buffer that is used as a temporary storage
	// TODO make this a handle to MemoryManager managed buffer
	struct ScratchBuffer
	{
		uint64_t deviceAddress = 0;
		VkBuffer handle = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};


	class IRayTracingSupport 
	{
	public:
		uint64_t getBufferDeviceAddress(Device& device, VkBuffer buffer);
		void createAccelerationStructure(Device& device, AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
		ScratchBuffer createScratchBuffer(Device& device, VkDeviceSize size);
		void deleteScratchBuffer(Device& device, ScratchBuffer& scratchBuffer);
		void deleteAccelerationStructure(Device& device, AccelerationStructure& accelerationStructure);

		// Function pointers for ray tracing related stuff
		PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
		PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
		PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
		PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
		PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
		PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
		PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
		PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
		PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
		PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

		void loadFunctionPointers(Device& device);
	};
}