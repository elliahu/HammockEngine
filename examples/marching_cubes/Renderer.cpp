#include "Renderer.h"
#include "MarchingCubes.h"

Renderer::Renderer():
window{instance, "Cloud Renderer", 1920, 1080},
device(instance, window.getSurface()) {
    init();
}

void Renderer::init() {

    // Resources
    descriptorSets.resize(Hammock::SwapChain::MAX_FRAMES_IN_FLIGHT);
    buffers.resize(Hammock::SwapChain::MAX_FRAMES_IN_FLIGHT);

    descriptorSetLayout = deviceStorage.createDescriptorSetLayout({
        .bindings = {
            {
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
            {
                .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS
            },
        }
    });

    for (int i = 0; i < Hammock::SwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        buffers[i] = deviceStorage.createBuffer({
                    .instanceSize = sizeof(BufferData),
                    .instanceCount = 1,
                    .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    .memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                });
    }

    for (int i = 0; i < Hammock::SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        auto fbufferInfo = deviceStorage.getBuffer(buffers[i])->descriptorInfo();
        descriptorSets[i] = deviceStorage.createDescriptorSet({
            .descriptorSetLayout = descriptorSetLayout,
            .bufferWrites = {{0, fbufferInfo}}
        });
    }

    // Load particles
    std::vector<Particle> particles;
    Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Loading particles...\n");
    if (loadParticles(assetPath("sph/sph_001000.bin"), particles)) {
        Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Loaded %d particles\n", particles.size());
    } else {
        Hammock::Logger::log(Hammock::LOG_LEVEL_ERROR, "Failed to load particles\n");
        throw std::runtime_error("Failed to load particles");
    }


    // Create a scalar field
    const float gridSize = 1.0f;   // The resolution of the grid
    const float fieldSize = 40.0f; // The size of the field (bounding box of the particles)
    Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Creating scalar field...\n");
    auto scalarField = createScalarField(particles, gridSize, fieldSize);
    Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Created scalar field of %d x %d x %d\n", scalarField.size(), scalarField[0].size(), scalarField[0][0].size());

    // marching cubes
    float isovalue = 7.0f; // Threshold value for surface extraction
    float cubeSize = 1.0f; // Size of the cubes in the marching cubes algorithm
    Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Marching cubes...\n");
    std::vector<Hammock::Triangle> triangles = marchingCubes(scalarField, isovalue, cubeSize);
    Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Marched surface of %d triangles\n", triangles.size());

    // create buffers
    for (const auto& triangle : triangles) {
        geometry.vertices.push_back(triangle.v0);
        geometry.vertices.push_back(triangle.v1);
        geometry.vertices.push_back(triangle.v2);
    }
    vertexBuffer = deviceStorage.createVertexBuffer({
        .vertexSize = sizeof(geometry.vertices[0]),
        .vertexCount = static_cast<uint32_t>(geometry.vertices.size()),
        .data = static_cast<void *>(geometry.vertices.data())
    });


    Hammock::Logger::log(Hammock::LOG_LEVEL_DEBUG, "Vertex buffer created with %d vertices \n", geometry.vertices.size());


    // pipeline
    pipeline = Hammock::GraphicsPipeline::createGraphicsPipelinePtr({
        .debugName = "marching_cubes",
        .device = device,
        .VS
        {
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("marching_cubes.vert")),
            .entryFunc = "main"
        },
        .FS
        {
            .byteCode = Hammock::Filesystem::readFile(compiledShaderPath("marching_cubes.frag")),
            .entryFunc = "main"
        },
        .descriptorSetLayouts =
        {
            deviceStorage.getDescriptorSetLayout(descriptorSetLayout).getDescriptorSetLayout()
        },
        .pushConstantRanges{
            {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(PushData)
            }
        },
        .graphicsState
        {
            .depthTest = VK_TRUE,
            .blendAtaAttachmentStates{},
            .vertexBufferBindings
            {
                .vertexBindingDescriptions = Hammock::Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Hammock::Vertex::vertexInputAttributeDescriptions()
            }
        },
        .renderPass = renderContext.getSwapChainRenderPass()
    });

}
