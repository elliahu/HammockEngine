#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

struct PointLight
{
    vec4 position;
    vec4 color;
};

layout (set = 0, binding = 0) uniform GlobalUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor; // w is intensity
    vec4 directionalLightDirection; // w is intenisty
    vec4 directionalLightColor; // w ignored
    PointLight pointLights[10];
    int numLights;
} ubo;

layout(push_constant) uniform Push
{
    vec2 x;
    vec2 y;
    vec2 z;
} push;


void main()
{
    outColor = vec4(1.0 , 0.0, 0.0, 1.0);
}
