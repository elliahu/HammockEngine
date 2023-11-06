#include "HmckCollisionDetectionSystem.h"

bool Hmck::CollisionDetectionSystem::intersect(GameObject& obj1, GameObject& obj2)
{
	assert(obj1.boundingBoxComponent != nullptr && "Provided object 1 needs bounding box to calculate intersections !");
	assert(obj2.boundingBoxComponent != nullptr && "Provided object 2 needs bounding box to calculate intersections !");

	return (
		obj1.boundingBoxComponent->x.min + obj1.transformComponent.translation.x <= obj2.boundingBoxComponent->x.max + obj2.transformComponent.translation.x &&
		obj1.boundingBoxComponent->x.max + obj1.transformComponent.translation.x >= obj2.boundingBoxComponent->x.min + obj2.transformComponent.translation.x &&
		obj1.boundingBoxComponent->y.min + obj1.transformComponent.translation.y <= obj2.boundingBoxComponent->y.max + obj2.transformComponent.translation.y &&
		obj1.boundingBoxComponent->y.max + obj1.transformComponent.translation.y >= obj2.boundingBoxComponent->y.min + obj2.transformComponent.translation.y &&
		obj1.boundingBoxComponent->z.min + obj1.transformComponent.translation.z <= obj2.boundingBoxComponent->z.max + obj2.transformComponent.translation.z &&
		obj1.boundingBoxComponent->z.max + obj1.transformComponent.translation.z >= obj2.boundingBoxComponent->z.min + obj2.transformComponent.translation.z
	);
}
