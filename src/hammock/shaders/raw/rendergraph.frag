#version 450

layout (location = 0) out vec4 finalColor;

layout (set = 0, binding = 0) uniform SceneUbo {
    mat4 mvp;
} ubo;

void main(){
    finalColor = vec4(1.0);
}