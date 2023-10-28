#version 450

// inputs
layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 position;
layout (location = 3) in vec4 tangent;

// outputs
layout (location = 0) out vec4 outColor;


layout(set = 1, binding = 0) uniform sampler2D albedoSampler;
layout(set = 1, binding = 1) uniform sampler2D normSampler;
layout(set = 1, binding = 2) uniform sampler2D roughMetalSampler;
layout(set = 1, binding = 3) uniform sampler2D occlusionSampler;


layout (set = 0, binding = 0) uniform ShaderData
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
} data;

// push constants
layout (push_constant) uniform Push
{
    mat4 modelMatrix; // model matrix
    mat4 normalMatrix; // using mat4 bcs alignment requirements
} push;

float beers_law(float distance, float absorbtion)
{
    return exp(-distance * absorbtion);
}



void main()
{
    vec3 sphere_position = vec3(2,0,2);


    outColor = texture(albedoSampler, uv) ;//+ vec4(1) * (1 - beers_law(distance(vec3(0), position),0.04));
}
