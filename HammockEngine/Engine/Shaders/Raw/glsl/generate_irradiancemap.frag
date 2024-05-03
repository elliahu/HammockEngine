#version 450

#define PI 3.14159265359

layout(set = 0, binding = 0) uniform sampler2D envMap;

layout(push_constant) uniform PushConsts {
	float resX;
    float resY;
} consts;

layout (location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

vec3 getEnvironmentColor(vec3 direction) {
    return texture(envMap, vec2(atan(direction.z, direction.x) / (2.0 * PI) + 0.5, asin(direction.y) / PI + 0.5)).rgb;
}

void main() {
    // Convert UV coordinates to spherical coordinates
    float phi = texCoord.x * PI;
    float theta = texCoord.y * PI * 0.5;

    vec3 direction = vec3(cos(phi) * sin(theta), cos(theta), sin(phi) * sin(theta));

    // Sample the environment map and compute irradiance
    vec3 irradiance = getEnvironmentColor(direction);

    fragColor = vec4(irradiance, 1.0);
}