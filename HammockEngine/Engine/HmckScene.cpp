#include "HmckScene.h"

Hmck::Scene::Scene(SceneCreateInfo createInfo): device{createInfo.device}, memory{createInfo.memory}
{
	// create root
	auto e = std::make_shared<Entity3D>();
	e->name = "Root";
	root = e->id;
	entities.emplace(e->id, std::move(e));

	if (!createInfo.environment.empty())
	{
		environment = std::make_shared<Environment>();

		EnvironmentLoader loader{ device, memory, environment };
		loader.load(createInfo.environment);
	}
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

void Hmck::Scene::update()
{
	if (cameras.size() == 0) addDefaultCamera();
	if (activeCamera == 0) activeCamera = cameras[0];

	for (auto& idx : cameras)
	{
		auto camera = getCamera(idx);
		camera->setViewYXZ(camera->transform.translation, camera->transform.rotation);
	}
}


void Hmck::Scene::add(std::shared_ptr<Entity> entity)
{
	entities.emplace(entity->id, entity);
	getRoot()->children.push_back(entity->id);
	lastAdded = entity->id;
}

void Hmck::Scene::addDefaultCamera()
{
	auto camera = std::make_shared<Camera>();
	camera->setViewYXZ(camera->transform.translation, camera->transform.rotation);
	camera->setPerspectiveProjection(glm::radians(50.0f), 16.f / 9.f, 0.1f, 1000.f);
	add(camera);
	cameras.push_back(camera->id);
}

