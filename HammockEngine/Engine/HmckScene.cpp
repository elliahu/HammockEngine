#include "HmckScene.h"

Hmck::Scene::Scene(SceneCreateInfo createInfo): device{createInfo.device}
{
	// create root
	root = std::make_shared<Entity3D>();

	// load all files
	for (auto& fileInfo : createInfo.loadFiles)
	{
		loadFile(fileInfo);
	}

	// load skybox
	loadSkybox(createInfo.loadSkybox);
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

	skybox.destroy(device);
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

void Hmck::Scene::loadSkybox(SkyboxLoadSkyboxInfo loadInfo)
{
	if (loadInfo.filenames.size() > 0)
	{
		skybox.loadFromFiles(
			loadInfo.filenames,
			VK_FORMAT_R8G8B8A8_UNORM,
			device
		);
		skybox.createSampler(device);
		skybox.updateDescriptor();
		hasSkybox = true;
	}
	
}

