#version 450

// inputs ignored but bound
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 tangent;

layout (location = 0) out vec4 color;

layout (set = 0, binding = 0) uniform SceneUbo {
    mat4 mvp;
} ubo;

void main(){
    gl_Position = ubo.mvp * vec4(position, 1.0);
    color = vec4(1);
}