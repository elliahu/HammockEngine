#version 450
#extension GL_EXT_nonuniform_qualifier: enable
#define INVALID_TEXTURE 4294967295
// inputs
layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec4 inTangent;

// outputs
layout (location = 0) out vec4 accumulationBuffer;
layout (location = 1) out float transparencyWeightBuffer;

layout (set = 0, binding = 1) uniform sampler2D textures[];

layout (set = 2, binding = 0) uniform MaterialPropertyUbo
{
    vec4 baseColorFactor;
    uint baseColorTextureIndex;
    uint normalTextureIndex;
    uint metallicRoughnessTextureIndex;
    uint occlusionTextureIndex;
    float alphaMode;
    float metallicFactor;
    float roughnessFactor;
} material;

layout (constant_id = 1) const float nearPlane = 0.1;
layout (constant_id = 2) const float farPlane = 64.0;

float linearDepth(float depth)
{
    float z = depth * 2.0f - 1.0f;
    return (2.0f * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
    vec4 position = vec4(inPosition, linearDepth(gl_FragCoord.z));
    vec4 albedo = (material.baseColorTextureIndex == INVALID_TEXTURE) ? material.baseColorFactor : texture(textures[material.baseColorTextureIndex], inUv);
    float roughness = (material.metallicRoughnessTextureIndex == INVALID_TEXTURE) ? material.roughnessFactor : texture(textures[material.metallicRoughnessTextureIndex], inUv).g;
    float metallic = (material.metallicRoughnessTextureIndex == INVALID_TEXTURE) ? material.metallicFactor : texture(textures[material.metallicRoughnessTextureIndex], inUv).b;
    float ambientOcclusion = (material.occlusionTextureIndex == INVALID_TEXTURE) ? 1.0 : texture(textures[material.occlusionTextureIndex], inUv).r;

    // Opacity calculation
    float opacity = albedo.a; // Use the alpha channel of the base color texture or factor
    float depthWeight = exp(-0.01 * linearDepth(gl_FragCoord.z)); // Example scaling factor
    float weight = opacity * depthWeight;
    // Write to accumulation and transparency weight buffers
    accumulationBuffer = vec4(albedo.rgb * weight, 1.0); // Store weighted color contribution
    transparencyWeightBuffer = weight; // Store transparency weight
}