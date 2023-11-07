#include "HmckGLTF.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace gltf = tinygltf;

std::shared_ptr<Hmck::Entity> Hmck::Gltf::load(
	std::string filename,
	Device& device, 
	std::vector<Image>& images,
	std::vector<Material>& materials,
	std::vector<Texture>& textures,
	std::vector<Vertex>& vertices,
	std::vector<uint32_t>& indices,
	std::vector<std::shared_ptr<Entity>>& entities,
	bool binary
	)
{
	gltf::TinyGLTF loader;
	gltf::Model model;
	std::string err;
	std::string warn;


	bool loaded = false;
	if (binary) 
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

	const gltf::Scene& scene = model.scenes[0];
	for (size_t i = 0; i < scene.nodes.size(); i++) {
		loadImages(model, device, images);
		loadMaterials(model, device, materials);
		loadTextures(model, device, textures);
		const gltf::Node node = model.nodes[scene.nodes[i]];
		loadNode(node, model, nullptr, vertices, indices, entities);
	}

	return entities[0];
}


void Hmck::Gltf::loadImages(gltf::Model& input,Device& device, std::vector<Image>& images)
{
	// Images can be stored inside the glTF (which is the case for the sample model), so instead of directly
	// loading them from disk, we fetch them from the glTF loader and upload the buffers
	images.resize(input.images.size());
	for (size_t i = 0; i < input.images.size(); i++) {
		gltf::Image& glTFImage = input.images[i];
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
		images[i].uri = glTFImage.uri;
		images[i].name = glTFImage.name;
		images[i].texture.loadFromBuffer(buffer,bufferSize, glTFImage.width, glTFImage.height,device, VK_FORMAT_R8G8B8A8_UNORM);
		images[i].texture.createSampler(device);
		images[i].texture.updateDescriptor();
		if (deleteBuffer) {
			delete[] buffer;
		}
	}
}


void Hmck::Gltf::loadMaterials(gltf::Model& input, Device& device, std::vector<Material>& materials)
{
	materials.resize(input.materials.size());
	for (size_t i = 0; i < input.materials.size(); i++) 
	{
		// Only read the most basic properties required
		tinygltf::Material glTFMaterial = input.materials[i];
		// Get the base color factor
		if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
			materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
		}
		// Get base color texture index
		if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) 
		{
			materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
		}
		// Get normal texture index
		if (glTFMaterial.additionalValues.find("normalTexture") != glTFMaterial.additionalValues.end()) 
		{
			materials[i].normalTextureIndex = glTFMaterial.additionalValues["normalTexture"].TextureIndex();
		}
		// Get rough/metal texture index
		if (glTFMaterial.values.find("metallicRoughnessTexture") != glTFMaterial.values.end()) 
		{
			materials[i].metallicRoughnessTextureIndex = glTFMaterial.values["metallicRoughnessTexture"].TextureIndex();
		}
		// Get occlusion texture index
		if (glTFMaterial.additionalValues.find("occlusionTexture") != glTFMaterial.additionalValues.end())
		{
			materials[i].occlusionTexture = glTFMaterial.additionalValues["occlusionTexture"].TextureIndex();
		}
		materials[i].alphaMode = glTFMaterial.alphaMode;
		materials[i].alphaCutOff = (float)glTFMaterial.alphaCutoff;
		materials[i].doubleSided = glTFMaterial.doubleSided;
		materials[i].name = glTFMaterial.name;
	}
}

void Hmck::Gltf::loadTextures(gltf::Model& input, Device& device, std::vector<Texture>& textures)
{
	textures.resize(input.textures.size());
	for (size_t i = 0; i < input.textures.size(); i++) {
		textures[i].imageIndex = input.textures[i].source;
	}
}

void Hmck::Gltf::loadNode(
	const tinygltf::Node& inputNode, 
	const tinygltf::Model& input, 
	std::shared_ptr<Entity> parent, 
	std::vector<Vertex>& vertices,
	std::vector<uint32_t>& indices,
	std::vector<std::shared_ptr<Entity>>& entities)
{
	std::shared_ptr<Entity3D> entity = std::make_unique<Entity3D>();
	entity->transform = glm::mat4(1.0f);
	entity->parent = parent;
	entity->name = inputNode.name;

	// Get the local entity matrix
	// It's either made up from translation, rotation, scale or a 4x4 matrix
	if (inputNode.translation.size() == 3) {
		entity->transform = glm::translate(entity->transform, glm::vec3(glm::make_vec3(inputNode.translation.data())));
	}
	if (inputNode.rotation.size() == 4) {
		glm::quat q = glm::make_quat(inputNode.rotation.data());
		entity->transform *= glm::mat4(q);
	}
	if (inputNode.scale.size() == 3) {
		entity->transform = glm::scale(entity->transform, glm::vec3(glm::make_vec3(inputNode.scale.data())));
	}
	if (inputNode.matrix.size() == 16) {
		entity->transform = glm::make_mat4x4(inputNode.matrix.data());
	};


	// Load entity's children
	if (inputNode.children.size() > 0) 
	{
		for (size_t i = 0; i < inputNode.children.size(); i++) 
		{
			loadNode(input.nodes[inputNode.children[i]], input, entity,vertices, indices, entities);
		}
	}

	// If the entity contains mesh data, we load vertices and indices from the buffers
	// In glTF this is done via accessors and buffer views
	if (inputNode.mesh > -1) 
	{
		const gltf::Mesh mesh = input.meshes[inputNode.mesh];
		// Iterate through all primitives of this entity's mesh
		for (size_t i = 0; i < mesh.primitives.size(); i++)
		{
			const gltf::Primitive& glTFPrimitive = mesh.primitives[i];
			uint32_t firstIndex = static_cast<uint32_t>(indices.size());
			uint32_t vertexStart = static_cast<uint32_t>(vertices.size());
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
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					vertexCount = accessor.count;
				}
				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) 
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}
				// Get buffer data for vertex texture coordinates
				// glTF supports multiple sets, we only load the first one
				if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) 
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}
				// load tangents as well
				if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					tangentBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
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
					vertices.push_back(vert);
				}
			}
			// Indices
			{
				const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
				const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

				indexCount += static_cast<uint32_t>(accessor.count);

				// glTF supports different component types of indices
				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
				{
					const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
					for (size_t index = 0; index < accessor.count; index++) {
						indices.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
				{
					const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
					for (size_t index = 0; index < accessor.count; index++) {
						indices.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
				{
					const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
					for (size_t index = 0; index < accessor.count; index++) {
						indices.push_back(buf[index] + vertexStart);
					}
					break;
				}
				default:
					std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
					throw std::runtime_error("Unsupported component type");
					return;
				}
			}
			Primitive primitive{};
			primitive.firstIndex = firstIndex;
			primitive.indexCount = indexCount;
			primitive.materialIndex = glTFPrimitive.material;
			entity->mesh.primitives.push_back(primitive);
		}
		entity->mesh.name = mesh.name;
	}

	if (parent) {
		parent->children.push_back(entity);
	}
	else {
		entities.push_back(entity);
	}
}
