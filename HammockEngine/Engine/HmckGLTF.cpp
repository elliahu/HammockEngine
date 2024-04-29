#include "HmckGLTF.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace gltf = tinygltf;

Hmck::GltfLoader::GltfLoader(GltfLoaderCreateInfo createInfo) :
	device{ createInfo.device },
	memory{ createInfo.memory },
	scene{ createInfo.scene }
{
	imagesOffset = scene->images.size();
	texturesOffset = scene->textures.size();
	materialsOffset = scene->materials.size();
}

void Hmck::GltfLoader::load(std::string filename)
{
	gltf::TinyGLTF loader;
	gltf::Model model;
	std::string err;
	std::string warn;


	bool loaded = false;
	if (isBinary(filename))
	{
		loaded = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
	}
	else
	{
		loaded = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	}

	if (!warn.empty())
	{
		std::cout << "glTF warinng: " << warn << "\n";
	}

	if (!err.empty())
	{
		std::cerr << "glTF ERROR: " << err << "\n";
	}

	if (!loaded) {
		std::cerr << "Failed to parse glTF\n";
	}


	loadImages(model);
	loadTextures(model);
	loadMaterials(model);


	loadEntities(model);
}

bool Hmck::GltfLoader::isBinary(std::string& filename)
{
	// Check if the file type is .gltf or .glb
	size_t dotPos = filename.find_last_of(".");
	if (dotPos != std::string::npos) {
		std::string extension = filename.substr(dotPos + 1);
		if (extension == "gltf") {
			return false;
		}
		else if (extension == "glb") {
			return true;
		}
		else {
			// Unsupported file type
			throw std::runtime_error("Unsupported file type: " + extension);
		}
	}
	else {
		// No file extension found
		throw std::runtime_error("No file extension found in the filename: " + filename);
	}
}

bool Hmck::GltfLoader::isLight(gltf::Node& node, gltf::Model& model)
{
	for (const auto& light : model.lights)
	{
		if (light.name == node.name) return true;
	}
	return false;
}

bool Hmck::GltfLoader::isSolid(gltf::Node& node)
{
	return node.mesh > -1;
}

void Hmck::GltfLoader::loadImages(gltf::Model& model)
{
	// Images can be stored inside the glTF, so instead of directly
	// loading them from disk, we fetch them from the glTF loader and upload the buffers
	scene->images.resize(model.images.size() + imagesOffset);
	for (size_t i = imagesOffset; i < model.images.size() + imagesOffset; i++) {
		gltf::Image& glTFImage = model.images[i - imagesOffset];
		// Get the image data from the glTF loader
		unsigned char* buffer = nullptr;
		uint32_t bufferSize = 0;
		bool deleteBuffer = false;
		// We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
		if (glTFImage.component == 3) {
			throw std::runtime_error("3-component images not supported");
		}
		else
		{
			buffer = &glTFImage.image[0];
			bufferSize = glTFImage.image.size();
		}
		// Load texture from image buffer
		scene->images[i].uri = glTFImage.uri;
		scene->images[i].name = glTFImage.name;
		scene->images[i].texture = memory.createTexture2DFromBuffer({
			.buffer = buffer,
			.bufferSize = bufferSize,
			.width = static_cast<uint32_t>(glTFImage.width),
			.height = static_cast<uint32_t>(glTFImage.height),
			.format = VK_FORMAT_R8G8B8A8_UNORM
			});
		if (deleteBuffer) {
			delete[] buffer;
		}
	}
}

void Hmck::GltfLoader::loadTextures(gltf::Model& model)
{
	scene->textures.resize(model.textures.size() + texturesOffset);
	for (size_t i = texturesOffset; i < model.textures.size() + texturesOffset; i++) {
		scene->textures[i].imageIndex = model.textures[i - texturesOffset].source + imagesOffset;
	}
}

void Hmck::GltfLoader::loadMaterials(gltf::Model& model)
{
	// if there is no material a default one will be created
	if (model.materials.size() == 0)
	{
		scene->materials.push_back({});
		return;
	}

	scene->materials.resize(model.materials.size() + materialsOffset);
	for (size_t i = materialsOffset; i < model.materials.size() + materialsOffset; i++)
	{
		// Only read the most basic properties required
		tinygltf::Material glTFMaterial = model.materials[i - materialsOffset];
		// Get the base color factor
		if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end())
		{
			scene->materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
		}
		// Get metallic factor
		if (glTFMaterial.values.find("metallicFactor") != glTFMaterial.values.end())
		{
			scene->materials[i].metallicFactor = glTFMaterial.values["metallicFactor"].Factor();
		}
		// Get roughness factor
		if (glTFMaterial.values.find("roughnessFactor") != glTFMaterial.values.end())
		{
			scene->materials[i].roughnessFactor = glTFMaterial.values["roughnessFactor"].Factor();
		}
		// Get base color texture index
		if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end())
		{
			scene->materials[i].baseColorTextureIndex = texturesOffset + glTFMaterial.values["baseColorTexture"].TextureIndex();
		}
		// Get normal texture index
		if (glTFMaterial.additionalValues.find("normalTexture") != glTFMaterial.additionalValues.end())
		{
			scene->materials[i].normalTextureIndex = texturesOffset + glTFMaterial.additionalValues["normalTexture"].TextureIndex();
		}
		// Get rough/metal texture index
		if (glTFMaterial.values.find("metallicRoughnessTexture") != glTFMaterial.values.end())
		{
			scene->materials[i].metallicRoughnessTextureIndex = texturesOffset + glTFMaterial.values["metallicRoughnessTexture"].TextureIndex();
		}
		// Get occlusion texture index
		if (glTFMaterial.additionalValues.find("occlusionTexture") != glTFMaterial.additionalValues.end())
		{
			scene->materials[i].occlusionTextureIndex = texturesOffset + glTFMaterial.additionalValues["occlusionTexture"].TextureIndex();
		}
		scene->materials[i].alphaMode = glTFMaterial.alphaMode;
		scene->materials[i].alphaCutOff = (float)glTFMaterial.alphaCutoff;
		scene->materials[i].doubleSided = glTFMaterial.doubleSided;
		scene->materials[i].name = glTFMaterial.name;
	}
}

void Hmck::GltfLoader::loadEntities(gltf::Model& model)
{
	const gltf::Scene& s = model.scenes[0];
	for (size_t i = 0; i < s.nodes.size(); i++) 
	{
		gltf::Node& node = model.nodes[s.nodes[i]];
		loadEntitiesRecursive(node, model, scene->getRoot()->id);
	}
}

void Hmck::GltfLoader::loadEntitiesRecursive(gltf::Node& node, gltf::Model& model, EntityId parent)
{
	// Load the current node
	if (isSolid(node)) 
	{
		loadEntity3D(node, model, parent);
	}
	else if (isLight(node, model)) 
	{
		loadIOmniLight(node, model, parent);
	}
	else 
	{
		loadEntity(node, model, parent);
	}

	// Load children recursively
	for (size_t i = 0; i < node.children.size(); i++) {
		gltf::Node& childNode = model.nodes[node.children[i]];
		loadEntitiesRecursive(childNode, model, scene->entities.back()->id);
	}
}

void Hmck::GltfLoader::loadEntity(gltf::Node& node, gltf::Model& model, EntityId parent)
{
	std::shared_ptr<Entity> entity = std::make_shared<Entity>();
	entity->parent = parent;
	entity->name = node.name;

	scene->entities.push_back(entity);


	// Get the local entity matrix
	// It's either made up from translation, rotation, scale or a 4x4 matrix
	if (node.translation.size() == 3) {
		entity->transform.translation = glm::vec3(glm::make_vec3(node.translation.data()));
	}
	if (node.rotation.size() == 4) {
		entity->transform.rotation = glm::eulerAngles(glm::make_quat(node.rotation.data()));
	}
	if (node.scale.size() == 3) {
		entity->transform.scale = glm::vec3(glm::make_vec3(node.scale.data()));
	}
	if (node.matrix.size() == 16) {
		glm::mat4 transform = glm::make_mat4x4(node.matrix.data());
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(
			transform,
			scale,
			rotation,
			translation,
			skew,
			perspective);
		entity->transform.scale = scale;
		entity->transform.rotation = glm::eulerAngles(rotation);
		entity->transform.translation = translation;
	};

	if (parent > 0) {
		scene->getEntity(parent)->children.push_back(entity->id);
		entity->parent = parent;
	}


}

void Hmck::GltfLoader::loadEntity3D(gltf::Node& node, gltf::Model& model, EntityId parent)
{
	std::shared_ptr<Entity3D> entity = std::make_shared<Entity3D>();
	entity->parent = parent;
	entity->name = node.name;
	scene->entities.push_back(entity);


	// Get the local entity matrix
	// It's either made up from translation, rotation, scale or a 4x4 matrix
	if (node.translation.size() == 3) {
		entity->transform.translation = glm::vec3(glm::make_vec3(node.translation.data()));
	}
	if (node.rotation.size() == 4) {
		entity->transform.rotation = glm::eulerAngles(glm::make_quat(node.rotation.data()));
	}
	if (node.scale.size() == 3) {
		entity->transform.scale = glm::vec3(glm::make_vec3(node.scale.data()));
	}
	if (node.matrix.size() == 16) {
		glm::mat4 transform = glm::make_mat4x4(node.matrix.data());
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(
			transform,
			scale,
			rotation,
			translation,
			skew,
			perspective);
		entity->transform.scale = scale;
		entity->transform.rotation = glm::eulerAngles(rotation);
		entity->transform.translation = translation;
	};

	const gltf::Mesh mesh = model.meshes[node.mesh];
	// Iterate through all primitives of this entity's mesh
	for (size_t i = 0; i < mesh.primitives.size(); i++)
	{
		const gltf::Primitive& glTFPrimitive = mesh.primitives[i];
		uint32_t firstIndex = static_cast<uint32_t>(scene->indices.size());
		uint32_t vertexStart = static_cast<uint32_t>(scene->vertices.size());
		uint32_t indexCount = 0;
		// Vertices
		{
			const float* positionBuffer = nullptr;
			const float* normalsBuffer = nullptr;
			const float* texCoordsBuffer = nullptr;
			const float* tangentBuffer = nullptr;
			size_t vertexCount = 0;

			// Get buffer data for vertex positions
			if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end())
			{
				const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("POSITION")->second];
				const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
				positionBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				vertexCount = accessor.count;
			}
			// Get buffer data for vertex normals
			if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end())
			{
				const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
				const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
				normalsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
			}
			// Get buffer data for vertex texture coordinates
			// glTF supports multiple sets, we only load the first one
			if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end())
			{
				const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
				const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
				texCoordsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
			}
			// load tangents as well
			if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end())
			{
				const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
				const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
				tangentBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
			}


			// Append data
			for (size_t v = 0; v < vertexCount; v++)
			{
				Vertex vert{};
				vert.position = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
				vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
				vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
				vert.color = glm::vec3(1.0f);
				vert.tangent = vert.tangent = tangentBuffer ? glm::make_vec4(&tangentBuffer[v * 4]) : glm::vec4(0.0f);
				scene->vertices.push_back(vert);
			}
		}
		// Indices
		{
			const tinygltf::Accessor& accessor = model.accessors[glTFPrimitive.indices];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			indexCount += static_cast<uint32_t>(accessor.count);

			// glTF supports different component types of indices
			switch (accessor.componentType) {
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
			{
				const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
				for (size_t index = 0; index < accessor.count; index++) {
					scene->indices.push_back(buf[index] + vertexStart);
				}
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
			{
				const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
				for (size_t index = 0; index < accessor.count; index++) {
					scene->indices.push_back(buf[index] + vertexStart);
				}
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
			{
				const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
				for (size_t index = 0; index < accessor.count; index++) {
					scene->indices.push_back(buf[index] + vertexStart);
				}
				break;
			}
			default:
				std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
				throw std::runtime_error("Unsupported component type");
			}
		}
		Primitive primitive{};
		primitive.firstIndex = firstIndex;
		primitive.indexCount = indexCount;
		primitive.materialIndex = materialsOffset + glTFPrimitive.material;
		entity->mesh.primitives.push_back(primitive);
	}
	entity->mesh.name = mesh.name;

	if (parent > 0) {
		scene->getEntity(parent)->children.push_back(entity->id);
		entity->parent = parent;
	}


}

void Hmck::GltfLoader::loadIOmniLight(gltf::Node& node, gltf::Model& model, EntityId parent)
{
	std::shared_ptr<OmniLight> light = std::make_shared<OmniLight>();
	light->parent = parent;
	light->name = node.name;

	scene->entities.push_back(light);

	if (node.translation.size() == 3) {
		light->transform.translation = glm::vec3(glm::make_vec3(node.translation.data()));
	}
	if (node.rotation.size() == 4) {
		light->transform.rotation = glm::eulerAngles(glm::make_quat(node.rotation.data()));
	}
	if (node.scale.size() == 3) {
		light->transform.scale = glm::vec3(glm::make_vec3(node.scale.data()));
	}
	if (node.matrix.size() == 16) {
		glm::mat4 transform = glm::make_mat4x4(node.matrix.data());
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(
			transform,
			scale,
			rotation,
			translation,
			skew,
			perspective);
		light->transform.scale = scale;
		light->transform.rotation = glm::eulerAngles(rotation);
		light->transform.translation = translation;
	};

	for (const auto& l : model.lights)
	{
		if (l.name == light->name)
		{
			light->color = { l.color[0], l.color[1], l.color[2] };
		}
	}

	if (parent > 0) {
		scene->getEntity(parent)->children.push_back(light->id);
		light->parent = parent;
	}
}


