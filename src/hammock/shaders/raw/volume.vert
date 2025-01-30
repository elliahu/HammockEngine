#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 tangent;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 cloudTransform;    // World space transform for the cloud volume
} pushConstants;

void main() {
    // Position in clip space (correct)
    gl_Position = camera.proj * camera.view * pushConstants.cloudTransform * vec4(position, 1.0);

}