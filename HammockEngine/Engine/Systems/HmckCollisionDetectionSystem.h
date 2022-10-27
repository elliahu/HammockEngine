#pragma once

#include "HmckDevice.h"
#include "HmckPipeline.h"
#include "HmckFrameInfo.h"
#include "HmckBuffer.h"

#include <vector>
#include <utility>


namespace Hmck
{
	class HmckCollisionDetectionSystem
	{
	public:

		// delete copy constructor and copy destructor
		HmckCollisionDetectionSystem(const HmckCollisionDetectionSystem&) = delete;
		HmckCollisionDetectionSystem& operator=(const HmckCollisionDetectionSystem&) = delete;

		
		bool intersect(HmckGameObject& obj1, HmckGameObject& obj2);
	};
}