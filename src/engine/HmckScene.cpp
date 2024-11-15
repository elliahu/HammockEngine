#include "HmckScene.h"


Hmck::Scene::Scene(SceneCreateInfo createInfo): device{createInfo.device}, memory{createInfo.memory}
{
	// create root
	auto e = std::make_shared<Entity3D>();
	e->name = "Root";
	root = e->id;
	entities.emplace(e->id, std::move(e));
	environment = std::make_shared<Environment>();
}

Hmck::Scene::~Scene()
{
	destroy();
}

void Hmck::Scene::destroy()
{
	for (unsigned int i = 0; i < images.size(); i++)
	{
		memory.destroyTexture2D(images[i].texture);
	}

	environment->destroy(memory);
}

void Hmck::Scene::add(std::shared_ptr<Entity> entity, std::shared_ptr<Entity> parent)
{
	entities.emplace(entity->id, entity);
	lastAdded = entity->id;
	if (isInstanceOf<Entity, Camera>(entity))
	{
		cameras.push_back(entity->id);
	}
	if (isInstanceOf<Entity, OmniLight>(entity))
	{
		lights.push_back(entity->id);
	}
	parent->children.push_back(entity);
	entity->parent = parent;
}


