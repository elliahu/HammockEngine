#include "HmckScene.h"

Hmck::Scene::Scene(SceneCreateInfo createInfo): device{createInfo.device}
{
	// create root
	root = std::make_shared<Entity3D>();
	root->name = "Scene root";

	// load all files
	for (auto& fileInfo : createInfo.loadFiles)
	{
		loadFile(fileInfo);
	}
	
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
		images[i].texture.destroy(device);
	}

	skyboxTexture.destroy(device);
}


void Hmck::Scene::loadFile(SceneLoadFileInfo loadInfo)
{
	// load the entity tree
	std::vector<std::shared_ptr<Entity>> roots = Gltf::load(
		loadInfo.filename, 
		device, 
		images,
		static_cast<uint32_t>(images.size()),
		materials, 
		static_cast<uint32_t>(materials.size()),
		textures, 
		static_cast<uint32_t>(textures.size()),
		vertices, 
		indices, 
		root,
		loadInfo.binary);
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

std::shared_ptr<Hmck::Entity> Hmck::Scene::getEntity(Hmck::Id id)
{
	if (id == 0)
		return root;
	return root->getChild(id);
}

