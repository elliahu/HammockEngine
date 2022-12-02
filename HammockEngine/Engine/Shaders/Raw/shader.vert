#version 450

// inputs
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;
layout (location = 4) in vec3 tangent;

//outputs
layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragPosWorld;
layout (location = 2) out vec3 fragNormalWorld;
layout (location = 3) out vec2 texCoords;
layout (location = 4) out mat3 TBN;

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
    vec4 positionWorld = push.modelMatrix * vec4(position, 1.0);

    gl_Position = ubo.projection * ubo.view *  positionWorld;
    
    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
    texCoords = uv;

    vec3 T = normalize(mat3(push.normalMatrix) * tangent);
    vec3 N = normalize(mat3(push.normalMatrix) * normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = transpose(mat3(T, B, N)); 
}