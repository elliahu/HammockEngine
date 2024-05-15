#include "HmckEntity.h"

Hmck::EntityHandle Hmck::Entity::currentId = 1;

glm::mat4 Hmck::Entity::mat4()
{
	glm::mat4 model = transform.mat4();
	std::shared_ptr<Entity> currentParent = parent;
	while (currentParent)
	{
		model = currentParent->transform.mat4() * model;
		currentParent = currentParent->parent;
	}

	return model;
}
