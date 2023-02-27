#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

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
    mat4 depthBiasMVP;
    vec4 ambientLightColor; // w is intensity
    DirectionalLight directionalLight;
    PointLight pointLights[10];
    int numLights;
} ubo;

layout(push_constant) uniform Push
{
    vec4 position;
    vec4 color;
    float radius;
} push;

const float M_PI = 3.1415926538;

void main()
{
    float dis = sqrt(dot(fragOffset, fragOffset));

    if(dis > 1.0)
    {
        discard;
    }

    float cosDis = 0.2 * (cos(dis * M_PI) + 1.0);
    outColor = vec4(push.color.xyz + cosDis, cosDis);
}