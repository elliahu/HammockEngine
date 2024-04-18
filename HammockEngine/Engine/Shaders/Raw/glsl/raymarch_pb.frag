#version 450

// inputs
layout (location = 0) in vec2 inUv;

// outputs
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform SceneUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
} scene;

layout(set = 0, binding = 1) uniform sampler2D noiseSampler;

layout (push_constant) uniform PushConstants {
    vec2 resolution;
    float elapsedTime;
} push;

void main()
{

}