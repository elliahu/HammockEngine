#include "HmckCollisionDetectionSystem.h"

bool Hmck::HmckCollisionDetectionSystem::intersect(HmckGameObject& obj1, HmckGameObject& obj2)
{
	assert(obj1.boundingBox != nullptr && "Provided object 1 needs bounding box to calculate intersections !");
	assert(obj2.boundingBox != nullptr && "Provided object 2 needs bounding box to calculate intersections !");

	return (
		obj1.boundingBox->x.min + obj1.transform.translation.x <= obj2.boundingBox->x.max + obj2.transform.translation.x &&
		obj1.boundingBox->x.max + obj1.transform.translation.x >= obj2.boundingBox->x.min + obj2.transform.translation.x &&
		obj1.boundingBox->y.min + obj1.transform.translation.y <= obj2.boundingBox->y.max + obj2.transform.translation.y &&
		obj1.boundingBox->y.max + obj1.transform.translation.y >= obj2.boundingBox->y.min + obj2.transform.translation.y &&
		obj1.boundingBox->z.min + obj1.transform.translation.z <= obj2.boundingBox->z.max + obj2.transform.translation.z &&
		obj1.boundingBox->z.max + obj1.transform.translation.z >= obj2.boundingBox->z.min + obj2.transform.translation.z
	);
}
