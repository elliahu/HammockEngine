#version 450

// inputs
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 tangent;

// outputs
layout (location = 0) out vec3 _normal;
layout (location = 1) out vec2 _uv;
layout (location = 2) out vec3 _position;
layout (location = 3) out vec4 _tangent;



layout (set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec4 pos;
    vec4 lightDir;
    vec4 lightColor;
    vec4 baseSkyColor;
    vec4 gradientSkyColor;
    int width;
    int height;
    float sunFactor;
    float sunExp;
} camera;



void main() {
    gl_Position = camera.proj * camera.view * vec4(position, 1.0);

    _position = position;
    _normal = normal;
    _tangent = tangent;
    _uv = uv;
}