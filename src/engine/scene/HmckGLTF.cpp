#include "HmckGLTF.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "HmckEntity3D.h"
#include "HmckLights.h"
#include "utils/HmckLogger.h"

namespace gltf = tinygltf;

Hmck::GltfLoader::GltfLoader(Device &device, ResourceManager &memory, std::unique_ptr<Scene> &scene) : device{device},
    resources{memory},
    scene{scene} {
    imagesOffset = scene->images.size();
    texturesOffset = scene->textures.size();
    materialsOffset = scene->materials.size();
}

void Hmck::GltfLoader::load(const std::string &filename, uint32_t fileLoadingFlags) {
    gltf::TinyGLTF loader;
    gltf::Model model;
    std::string err;
    std::string warn;

    bool loaded = false;
    if (isBinary(filename)) {
        loaded = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    } else {
        loaded = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    }

    if (!warn.empty()) {
        Logger::log(HMCK_LOG_LEVEL_WARN, "glTF warinng: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        Logger::log(HMCK_LOG_LEVEL_ERROR, "glTF ERROR: %s\n", err.c_str());
        throw std::runtime_error{"glTF ERROR: " + err + "\n"};
    }

    if (!loaded) {
        Logger::log(HMCK_LOG_LEVEL_ERROR, "Failed to parse glTF\n");
        throw std::runtime_error{"Failed to parse glTF\n"};
    }

    imagesOffset = scene->images.size();
    texturesOffset = scene->textures.size();
    materialsOffset = scene->materials.size();

    loadingFlags = fileLoadingFlags;

    loadImages(model);
    loadTextures(model);
    loadMaterials(model);
    loadEntities(model);
}

bool Hmck::GltfLoader::isBinary(const std::string &filename) {
    // Check if the file type is .gltf or .glb
    if (const size_t dotPos = filename.find_last_of('.'); dotPos != std::string::npos) {
        if (const std::string extension = filename.substr(dotPos + 1); extension == "gltf") {
            return false;
        } else if (extension == "glb") {
            return true;
        } else {
            // Unsupported file type
            throw std::runtime_error("Unsupported file type: " + extension);
        }
    } else {
        // No file extension found
        throw std::runtime_error("No file extension found in the filename: " + filename);
    }
}

bool Hmck::GltfLoader::isLight(const gltf::Node &node, gltf::Model &model) {
    for (const auto &light: model.lights) {
        if (light.name == node.name) return true;
    }
    return false;
}

bool Hmck::GltfLoader::isSolid(const gltf::Node &node) {
    return node.mesh > -1;
}

bool Hmck::GltfLoader::isCamera(const gltf::Node &node) {
    return node.camera > -1;
}

void Hmck::GltfLoader::loadImages(gltf::Model &model) const {
    // Images can be stored inside the glTF, so instead of directly
    // loading them from disk, we fetch them from the glTF loader and upload the buffers
    scene->images.resize(model.images.size() + imagesOffset);
    for (size_t i = imagesOffset; i < model.images.size() + imagesOffset; i++) {
        gltf::Image &glTFImage = model.images[i - imagesOffset];
        // Get the image data from the glTF loader
        unsigned char *buffer = nullptr;
        // We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
        if (glTFImage.component == 3) {
            throw std::runtime_error("3-component images not supported");
        } else {
            buffer = &glTFImage.image[0];
        }
        // Load texture from image buffer
        scene->images[i].uri = glTFImage.uri;
        scene->images[i].name = glTFImage.name;
        // TODO load mip maps as well
        scene->images[i].texture = resources.createTexture2D({
            .buffer = static_cast<const void*>(buffer),
            .instanceSize = sizeof(unsigned char),
            .width = static_cast<uint32_t>(glTFImage.width),
            .height = static_cast<uint32_t>(glTFImage.height),
            .channels = 4,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .samplerInfo = {
                .maxLod = static_cast<float>(getNumberOfMipLevels(static_cast<uint32_t>(glTFImage.width), static_cast<uint32_t>(glTFImage.height))),
            }
        });
    }
}

void Hmck::GltfLoader::loadTextures(const gltf::Model &model) const {
    scene->textures.resize(model.textures.size() + texturesOffset);
    for (size_t i = texturesOffset; i < model.textures.size() + texturesOffset; i++) {
        scene->textures[i].imageIndex = model.textures[i - texturesOffset].source + imagesOffset;
    }
}

void Hmck::GltfLoader::loadMaterials(const gltf::Model &model) const {
    // if there is no material a default one will be created
    if (model.materials.empty()) {
        scene->materials.push_back({});
        return;
    }

    scene->materials.resize(model.materials.size() + materialsOffset);
    for (size_t i = materialsOffset; i < model.materials.size() + materialsOffset; i++) {
        // Only read the most basic properties required
        tinygltf::Material glTFMaterial = model.materials[i - materialsOffset];
        // Get the base color factor
        if (glTFMaterial.values.contains("baseColorFactor")) {
            scene->materials[i].baseColorFactor = glm::make_vec4(
                glTFMaterial.values["baseColorFactor"].ColorFactor().data());
        }
        // Get metallic factor
        if (glTFMaterial.values.contains("metallicFactor")) {
            scene->materials[i].metallicFactor = glTFMaterial.values["metallicFactor"].Factor();
        }
        // Get roughness factor
        if (glTFMaterial.values.contains("roughnessFactor")) {
            scene->materials[i].roughnessFactor = glTFMaterial.values["roughnessFactor"].Factor();
        }
        // Get base color texture index
        if (glTFMaterial.values.contains("baseColorTexture")) {
            scene->materials[i].baseColorTextureIndex =
                    texturesOffset + glTFMaterial.values["baseColorTexture"].TextureIndex();
        }
        // Get normal texture index
        if (glTFMaterial.additionalValues.contains("normalTexture")) {
            scene->materials[i].normalTextureIndex =
                    texturesOffset + glTFMaterial.additionalValues["normalTexture"].TextureIndex();
        }
        // Get rough/metal texture index
        if (glTFMaterial.values.contains("metallicRoughnessTexture")) {
            scene->materials[i].metallicRoughnessTextureIndex =
                    texturesOffset + glTFMaterial.values["metallicRoughnessTexture"].TextureIndex();
        }
        // Get occlusion texture index
        if (glTFMaterial.additionalValues.contains("occlusionTexture")) {
            scene->materials[i].occlusionTextureIndex =
                    texturesOffset + glTFMaterial.additionalValues["occlusionTexture"].TextureIndex();
        }
        scene->materials[i].alphaMode = glTFMaterial.alphaMode;
        scene->materials[i].alphaCutOff = static_cast<float>(glTFMaterial.alphaCutoff);
        scene->materials[i].doubleSided = glTFMaterial.doubleSided;
        scene->materials[i].name = glTFMaterial.name;
    }
}

void Hmck::GltfLoader::loadEntities(gltf::Model &model) {
    const gltf::Scene &s = model.scenes[0];
    for (size_t i = 0; i < s.nodes.size(); i++) {
        gltf::Node &node = model.nodes[s.nodes[i]];
        loadEntitiesRecursive(node, model, scene->getRoot());
    }
}

void Hmck::GltfLoader::loadEntitiesRecursive(gltf::Node &node, gltf::Model &model, const std::shared_ptr<Entity> &parent) {
    // Load the current node
    if (isSolid(node)) {
        loadEntity3D(node, model, parent);
    } else if (isLight(node, model)) {
        loadIOmniLight(node, model, parent);
    } else if (isCamera(node)) {
        loadCamera(node, model, parent);
    } else {
        loadEntity(node, model, parent);
    }

    // Load children recursively
    for (size_t i = 0; i < node.children.size(); i++) {
        gltf::Node &childNode = model.nodes[node.children[i]];
        loadEntitiesRecursive(childNode, model, scene->getEntity(scene->getLastAdded()));
    }
}

void Hmck::GltfLoader::loadEntity(const gltf::Node &node, gltf::Model &model, const std::shared_ptr<Entity> &parent) const {
    const auto entity = std::make_shared<Entity>();
    entity->parent = parent;
    entity->name = node.name;
    scene->add(entity, parent);


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
        const glm::mat4 transform = glm::make_mat4x4(node.matrix.data());
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
}

void Hmck::GltfLoader::loadEntity3D(gltf::Node &node, gltf::Model &model, std::shared_ptr<Entity> parent) const {
    auto entity = std::make_shared<Entity3D>();
    entity->parent = parent;
    entity->name = node.name;
    scene->add(entity, parent);


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
    for (size_t i = 0; i < mesh.primitives.size(); i++) {
        const gltf::Primitive &glTFPrimitive = mesh.primitives[i];
        auto firstIndex = static_cast<uint32_t>(scene->indices.size());
        auto vertexStart = static_cast<uint32_t>(scene->vertices.size());
        uint32_t indexCount = 0;
        // Vertices
        {
            const float *positionBuffer = nullptr;
            const float *normalsBuffer = nullptr;
            const float *texCoordsBuffer = nullptr;
            const float *tangentBuffer = nullptr;
            size_t vertexCount = 0;

            // Get buffer data for vertex positions
            if (glTFPrimitive.attributes.contains("POSITION")) {
                const tinygltf::Accessor &accessor = model.accessors[glTFPrimitive.attributes.find("POSITION")->second];
                const tinygltf::BufferView &view = model.bufferViews[accessor.bufferView];
                positionBuffer = reinterpret_cast<const float *>(&(model.buffers[view.buffer].data[
                    accessor.byteOffset + view.byteOffset]));
                vertexCount = accessor.count;
            }
            // Get buffer data for vertex normals
            if (glTFPrimitive.attributes.contains("NORMAL")) {
                const tinygltf::Accessor &accessor = model.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
                const tinygltf::BufferView &view = model.bufferViews[accessor.bufferView];
                normalsBuffer = reinterpret_cast<const float *>(&(model.buffers[view.buffer].data[
                    accessor.byteOffset + view.byteOffset]));
            }
            // Get buffer data for vertex texture coordinates
            // glTF supports multiple sets, we only load the first one
            if (glTFPrimitive.attributes.contains("TEXCOORD_0")) {
                const tinygltf::Accessor &accessor = model.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->
                    second];
                const tinygltf::BufferView &view = model.bufferViews[accessor.bufferView];
                texCoordsBuffer = reinterpret_cast<const float *>(&(model.buffers[view.buffer].data[
                    accessor.byteOffset + view.byteOffset]));
            }
            // load tangents as well
            if (glTFPrimitive.attributes.contains("TANGENT")) {
                const tinygltf::Accessor &accessor = model.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
                const tinygltf::BufferView &view = model.bufferViews[accessor.bufferView];
                tangentBuffer = reinterpret_cast<const float *>(&(model.buffers[view.buffer].data[
                    accessor.byteOffset + view.byteOffset]));
            }


            // Append data
            for (size_t v = 0; v < vertexCount; v++) {
                Vertex vert{};
                vert.position = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                vert.normal = glm::normalize(
                    glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                vert.color = glm::vec3(1.0f);
                vert.tangent = vert.tangent = tangentBuffer ? glm::make_vec4(&tangentBuffer[v * 4]) : glm::vec4(0.0f);

                if (loadingFlags & LoadingFlags::PreTransformVertices) {
                    glm::mat4 modelMat4 = entity->transform.mat4();
                    std::shared_ptr<Entity> currentParent = entity->parent;
                    while (currentParent) {
                        modelMat4 = currentParent->transform.mat4() * modelMat4;
                        currentParent = currentParent->parent;
                    }
                    vert.position = glm::vec3(modelMat4 * glm::vec4(vert.position, 1.0));
                    vert.normal = glm::normalize(glm::mat3(modelMat4) * vert.normal);
                    vert.tangent = glm::normalize(modelMat4 * vert.tangent);
                }

                if (loadingFlags & LoadingFlags::FlipY) {
                    vert.position.y *= -1.0f;
                    vert.normal.y *= -1.0f;
                    vert.tangent.y *= -1.0f;

                    // retain front face orientation
                    vert.position.x *= -1.0f;
                    vert.normal.x *= -1.0f;
                    vert.tangent.x *= -1.0f;
                }

                scene->vertices.push_back(vert);
            }
        }
        // Indices
        {
            const tinygltf::Accessor &accessor = model.accessors[glTFPrimitive.indices];
            const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

            indexCount += static_cast<uint32_t>(accessor.count);

            // glTF supports different component types of indices
            switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    const auto *buf = reinterpret_cast<const uint32_t *>(&buffer.data[
                        accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        scene->indices.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const auto *buf = reinterpret_cast<const uint16_t *>(&buffer.data[
                        accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        scene->indices.push_back(buf[index] + vertexStart);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const auto *buf = reinterpret_cast<const uint8_t *>(&buffer.data[
                        accessor.byteOffset + bufferView.byteOffset]);
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
        primitive.materialIndex = glTFPrimitive.material + materialsOffset;
        entity->mesh.primitives.push_back(primitive);
    }
    entity->mesh.name = mesh.name;
}

void Hmck::GltfLoader::loadIOmniLight(const gltf::Node &node, gltf::Model &model, const std::shared_ptr<Entity> &parent) const {
    const auto light = std::make_shared<OmniLight>();
    light->parent = parent;
    light->name = node.name;
    scene->add(light, parent);
    scene->lights.push_back(light->id);

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
        const glm::mat4 transform = glm::make_mat4x4(node.matrix.data());
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

    for (const auto &l: model.lights) {
        if (l.name == light->name) {
            light->color = {l.color[0], l.color[1], l.color[2]};
        }
    }
}

void Hmck::GltfLoader::loadCamera(const gltf::Node &node, const gltf::Model &model, const std::shared_ptr<Entity> &parent) const {
    const auto &cameraNode = model.cameras[node.camera];

    const auto camera = std::make_shared<Camera>();
    camera->parent = parent;
    camera->name = node.name;
    scene->add(camera, parent);
    scene->cameras.push_back(camera->id);

    if (node.translation.size() == 3) {
        camera->transform.translation = glm::vec3(glm::make_vec3(node.translation.data()));
    }
    if (node.rotation.size() == 4) {
        camera->transform.rotation = glm::eulerAngles(glm::make_quat(node.rotation.data()));
    }
    if (node.scale.size() == 3) {
        camera->transform.scale = glm::vec3(glm::make_vec3(node.scale.data()));
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
        camera->transform.scale = scale;
        camera->transform.rotation = glm::eulerAngles(rotation);
        camera->transform.translation = translation;
    };

    if (scene->activeCamera == 0) {
        scene->setActiveCamera(camera->id);
    }

    if (cameraNode.type == "perspective") {
        camera->setPerspectiveProjection(cameraNode.perspective.yfov, cameraNode.perspective.aspectRatio,
                                         cameraNode.perspective.znear, cameraNode.perspective.zfar);
    }


    if (cameraNode.type == "orthographic") {
        // Get the orthographic parameters
        const gltf::OrthographicCamera &ortho = cameraNode.orthographic;

        // Extract the bounds
        const float left = -1.0f * ortho.xmag; // Left bound
        const float right = ortho.xmag; // Right bound
        const float top = ortho.ymag; // Top bound
        const float bottom = ortho.ymag * -1.0f; // Bottom bound
        const float _near = ortho.znear; // Near bound
        const float _far = ortho.zfar; // Far bound
        camera->setOrthographicProjection(left, right, top, bottom, _near, _far);
    }
}
