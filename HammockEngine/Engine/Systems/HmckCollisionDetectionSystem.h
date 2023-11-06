#pragma once

#include "HmckDevice.h"
#include "HmckPipeline.h"
#include "HmckFrameInfo.h"
#include "HmckBuffer.h"

#include <vector>
#include <utility>


namespace Hmck
{
	class CollisionDetectionSystem
	{
	public:
		CollisionDetectionSystem(const CollisionDetectionSystem&) = delete;
		CollisionDetectionSystem& operator=(const CollisionDetectionSystem&) = delete;
		
		bool intersect(GameObject& obj1, GameObject& obj2);
	};
}