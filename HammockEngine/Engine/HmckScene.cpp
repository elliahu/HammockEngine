#include "HmckScene.h"

Hmck::Scene::Scene(SceneCreateInfo createInfo): device{createInfo.device}, memory{createInfo.memory}
{
	// create root
	auto e = std::make_shared<Entity3D>();
	e->name = "Root";
	root = e->id;
	entities.emplace(e->id, std::move(e));
	
	// load skybox
	if (createInfo.loadSkybox.textures.size() > 0)
	{
		loadSkyboxTexture(createInfo.loadSkybox);
		skyboxVertices = {
			{{-0.5f, -0.5f, -0.5f}},
			{{0.5f, -0.5f, -0.5f}},
			{{0.5f, 0.5f, -0.5f}},
			{{-0.5f, 0.5f, -0.5f}},
			{{-0.5f, -0.5f, 0.5f}},
			{{0.5f, -0.5f, 0.5f}},
			{{0.5f, 0.5f, 0.5f}},
			{{-0.5f, 0.5f, 0.5f}}
		};
		skyboxVertexCount = skyboxVertices.size();

		skyboxIndices = {
			2, 1, 0, 0, 3, 2, // Front face
			4, 5, 6, 6, 7, 4, // Back face
			6, 5, 1, 1, 2, 6, // Right face
			0, 4, 7, 7, 3, 0, // Left face
			6, 2, 3, 3, 7, 6, // Top face
			0, 1, 5, 5, 4, 0  // Bottom face
		};
		skyboxIndexCount = 36;
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

	skyboxTexture.destroy(device);
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

void Hmck::Scene::loadSkyboxTexture(SkyboxLoadSkyboxInfo loadInfo)
{
	skyboxTexture.loadFromFiles(
		loadInfo.textures,
		VK_FORMAT_R8G8B8A8_UNORM,
		device
	);
	skyboxTexture.createSampler(device);
	skyboxTexture.updateDescriptor();
	hasSkybox = true;
}

void Hmck::Scene::addDefaultCamera()
{
	auto camera = std::make_shared<Camera>();
	camera->setViewYXZ(camera->transform.translation, camera->transform.rotation);
	camera->setPerspectiveProjection(glm::radians(50.0f), 16.f / 9.f, 0.1f, 1000.f);
	add(camera);
	cameras.push_back(camera->id);
}

