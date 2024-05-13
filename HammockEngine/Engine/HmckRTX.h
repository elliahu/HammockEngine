#pragma once
// vulkan headers
#include <vulkan/vulkan.h>

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
}