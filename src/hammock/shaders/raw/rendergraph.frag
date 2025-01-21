#version 450
layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 finalColor;

layout (set = 0, binding = 0) uniform SceneUbo {
    mat4 mvp;
} ubo;

void main(){
    finalColor = vec4(uv.x, uv.y, 0.0, 1.0);
}