#version 450

// inputs
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;

//outputs
layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec3 fragPosWorld;
layout (location = 2) out vec3 fragNormalWorld;

// descriptor set
layout (set = 0, binding = 0) uniform GlobalUbo
{
    mat4 projectionViewMatrix;
    vec4 ambientLightColor; // w is intensity
    vec3 lightPosition;
    vec4 lightColor; // w is intensity
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

    gl_Position = ubo.projectionViewMatrix *  positionWorld;
    
    fragNormalWorld = normalize(mat3(push.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
}