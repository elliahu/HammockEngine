#version 450
#extension GL_EXT_nonuniform_qualifier : enable

#define INVALID_TEXTURE 4294967295


// inputs
layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 position;
layout (location = 3) in vec4 tangent;

// outputs
layout (location = 0) out vec4 outColor;


layout (set = 0, binding = 0) uniform SceneUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    mat4 depthBias;
    vec4 ambientColor;
} scene;

layout(set = 0, binding = 1) uniform sampler2D textures[];

layout (set = 0, binding = 2) uniform TransformUbo
{
    mat4 model;
    mat4 normal;
} transform;

layout (set = 0, binding = 3) uniform MaterialPropertyUbo
{
    vec4 baseColorFactor;
    uint baseColorTextureIndex;
    uint normalTextureIndex;
    uint metallicRoughnessTextureIndex;
    uint occlusionTextureIndex;
    float alphaCutoff;
} material;


float beers_law(float distance, float absorbtion)
{
    return exp(-distance * absorbtion);
}

void main()
{
    vec3 sphere_position = vec3(2,0,2);

    if(material.baseColorTextureIndex == INVALID_TEXTURE)
    {
        outColor = material.baseColorFactor;
    }
    else
    {
        outColor = texture(textures[material.baseColorTextureIndex], uv);
    }
    
}
