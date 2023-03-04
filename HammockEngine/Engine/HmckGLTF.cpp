#include "HmckGLTF.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace gltf = tinygltf;

Hmck::HmckGLTF::HmckGLTF(HmckDevice& device): device{device}
{
	descriptorPool = HmckDescriptorPool::Builder(device)
		.setMaxSets(100)
		.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 30) // 30 should be good
		.build();

	descriptorSetLayout = HmckDescriptorSetLayout::Builder(device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // albedo
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // normal
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // roughnessMetalic
		.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // occlusion
		.build();
}

Hmck::HmckGLTF::~HmckGLTF()
{
	for (unsigned int i = 0; i < images.size(); i++)
	{
		images[i].texture.destroy(device);
	}

	for (auto& material : materials) {
		vkDestroyPipeline(device.device(), material.pipeline, nullptr);
	}
}

void Hmck::HmckGLTF::load(std::string filepath, Config& info)
{
	gltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool loaded = false;
	if (info.binary) 
	{
		loaded = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
	}
	else 
	{
		loaded = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
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

	path = filepath;

	const gltf::Scene& scene = model.scenes[0];
	for (size_t i = 0; i < scene.nodes.size(); i++) {
		loadImages(model);
		loadMaterials(model);
		loadTextures(model);
		const gltf::Node node = model.nodes[scene.nodes[i]];
		loadNode(node, model, nullptr);
	}
	createVertexBuffer();
	createIndexBuffer();
	vertices.clear();
	indices.clear();
	prepareDescriptors();
}

void Hmck::HmckGLTF::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
	VkDeviceSize offsets[] = { 0 };
	VkBuffer buffers[] = { vertexBuffer->getBuffer() };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	// Render all nodes at top-level
	for (auto& node : nodes) {
		drawNode(commandBuffer, node, pipelineLayout);
	}
}

void Hmck::HmckGLTF::drawNode(VkCommandBuffer commandBuffer, std::shared_ptr<Node>& node, VkPipelineLayout pipelineLayout)
{
	// don't render invisible nodes
	if (!node->visible) { return; }

	if (node->mesh.primitives.size() > 0) {
		// Pass the node's matrix via push constants
		// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
		glm::mat4 nodeMatrix = node->matrix;
		std::shared_ptr<Node> currentParent = node->parent;
		while (currentParent) {
			nodeMatrix = currentParent->matrix * nodeMatrix;
			currentParent = currentParent->parent;
		}
		// Pass the final matrix to the vertex shader using push constants
		struct MatrixPushConstantData
		{
			glm::mat4 model;
			glm::mat4 normal;
		};

		MatrixPushConstantData pushData{};
		pushData.model = nodeMatrix;
		pushData.normal = glm::transpose(glm::inverse(nodeMatrix));
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MatrixPushConstantData), &pushData);

		for (Primitive& primitive : node->mesh.primitives) {
			if (primitive.indexCount > 0) {
				// Get the material index for this primitive
				Material& material = materials[primitive.materialIndex];
				// Bind the pipeline for the node's material
				//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipeline);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &material.descriptorSet, 0, nullptr);
				// draw
				vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
			}
		}
	}
	for (auto& child : node->children) {
		drawNode(commandBuffer, child, pipelineLayout);
	}
}

void Hmck::HmckGLTF::prepareDescriptors()
{
	for (auto& material : materials)
	{
		// create descriptor set
		HmckDescriptorWriter(*descriptorSetLayout, *descriptorPool)
			.writeImage(0, &images[textures[material.baseColorTextureIndex].imageIndex].texture.descriptor)
			.writeImage(1, &images[textures[material.normalTextureIndex].imageIndex].texture.descriptor)
			.writeImage(2, &images[textures[material.metallicRoughnessTexture].imageIndex].texture.descriptor)
			.writeImage(3, &images[textures[material.occlusionTexture].imageIndex].texture.descriptor)
			.build(material.descriptorSet);
	}
}

void Hmck::HmckGLTF::loadImages(gltf::Model& input)
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


void Hmck::HmckGLTF::loadMaterials(gltf::Model& input)
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
			materials[i].metallicRoughnessTexture = glTFMaterial.values["metallicRoughnessTexture"].TextureIndex();
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

void Hmck::HmckGLTF::loadTextures(gltf::Model& input)
{
	textures.resize(input.textures.size());
	for (size_t i = 0; i < input.textures.size(); i++) {
		textures[i].imageIndex = input.textures[i].source;
	}
}

void Hmck::HmckGLTF::loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, std::shared_ptr<Node> parent)
{
	std::shared_ptr<Node> node = std::make_unique<Node>();
	node->matrix = glm::mat4(1.0f);
	node->parent = parent;
	node->name = inputNode.name;

	// Get the local node matrix
	// It's either made up from translation, rotation, scale or a 4x4 matrix
	if (inputNode.translation.size() == 3) {
		node->matrix = glm::translate(node->matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
	}
	if (inputNode.rotation.size() == 4) {
		glm::quat q = glm::make_quat(inputNode.rotation.data());
		node->matrix *= glm::mat4(q);
	}
	if (inputNode.scale.size() == 3) {
		node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
	}
	if (inputNode.matrix.size() == 16) {
		node->matrix = glm::make_mat4x4(inputNode.matrix.data());
	};


	// Load node's children
	if (inputNode.children.size() > 0) 
	{
		for (size_t i = 0; i < inputNode.children.size(); i++) 
		{
			loadNode(input.nodes[inputNode.children[i]], input, node);
		}
	}

	// If the node contains mesh data, we load vertices and indices from the buffers
	// In glTF this is done via accessors and buffer views
	if (inputNode.mesh > -1) 
	{
		const gltf::Mesh mesh = input.meshes[inputNode.mesh];
		// Iterate through all primitives of this node's mesh
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
			node->mesh.primitives.push_back(primitive);
		}
		node->mesh.name = mesh.name;
	}

	if (parent) {
		parent->children.push_back(node);
	}
	else {
		nodes.push_back(node);
	}
}

void Hmck::HmckGLTF::createVertexBuffer()
{
	// copy data to staging memory on device, then copy from staging to v/i memory
	vertexCount = static_cast<uint32_t>(vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
	uint32_t vertexSize = sizeof(vertices[0]);

	HmckBuffer stagingBuffer{
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};


	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)vertices.data());

	vertexBuffer = std::make_unique<HmckBuffer>(
		device,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

	device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void Hmck::HmckGLTF::createIndexBuffer()
{
	indexCount = static_cast<uint32_t>(indices.size());

	VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
	uint32_t indexSize = sizeof(indices[0]);

	HmckBuffer stagingBuffer{
		device,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)indices.data());

	indexBuffer = std::make_unique<HmckBuffer>(
		device,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);


	device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}

