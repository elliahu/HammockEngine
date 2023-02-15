#version 450

//inputs
layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inLightPos;

// outputs
layout (location = 0) out float outColor;

struct PointLight
{
    vec4 position;
    vec4 color;
    vec4 terms;
};

struct DirectionalLight
{
    vec4 direction;
    vec4 color;
};

layout (set = 0, binding = 0) uniform GlobalUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor; // w is intensity
    DirectionalLight directionalLight;
    PointLight pointLights[10];
    int numLights;
} ubo;
 
// push constants
layout (push_constant) uniform Push
{
    mat4 modelMatrix; // model matrix
    mat4 normalMatrix; // using mat4 bcs alignment requirements
} push;


void main()
{
    // Store distance to light as 32 bit float value
    vec3 lightVec = inPos.xyz - inLightPos;
    outColor = length(lightVec);
}