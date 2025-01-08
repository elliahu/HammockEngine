#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 tangent;

layout (location = 0) out vec2 _uv;
layout (location = 1) out float _distance;

layout(binding = 0) uniform UniformBufferObject {
    mat4 projMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    vec4 cameraPos;
    vec4 lightDirection;
    vec4 lightColor;
} ubo;


void main() {
    _uv = vec2((position.x + 1.0) * 0.5, (1.0 - position.y) * 0.5);
    vec4 worldPosition = ubo.modelMatrix * vec4(position, 1.0);
    _distance = length(ubo.cameraPos.xyz - worldPosition.xyz);
    gl_Position = ubo.projMatrix * ubo.viewMatrix * worldPosition;
}