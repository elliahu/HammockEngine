#version 450

layout (location = 0) in vec2 inUv; // uv of the fragment
layout (location = 1) in float dist; // distance of the fragment from the camera
layout (location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 projMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    vec4 cameraPos;
    vec4 lightDirection;
    vec4 lightColor;
} ubo;

layout(binding = 1) uniform sampler3D volumeTexture;

layout (push_constant) uniform PushConstants {
    float stepSize;
    int maxSteps;
    float minDensityThreshold;
    float time;
} push;

// Function to compute ray direction
vec3 computeRayDirection(vec2 uv) {
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, -1.0, 1.0);
    vec4 eyeSpace = inverse(ubo.projMatrix) * clipSpace;
    eyeSpace.z = -1.0; // Ensure ray direction
    eyeSpace.w = 0.0;  // Vector, not point
    return normalize((inverse(ubo.viewMatrix) * eyeSpace).xyz);
}

// Function to perform raymarching
vec4 raymarch(vec3 rayOrigin, vec3 rayDir) {
    vec3 currentPosition = rayOrigin;
    for (int step = 0; step < push.maxSteps; ++step) {
        float density = texture(volumeTexture, currentPosition).r;

        // Early exit if density exceeds threshold
        if (density > push.minDensityThreshold) {
            vec3 color = vec3(density); // Simple grayscale based on density
            return vec4(color, density);
        }

        // Advance along the ray
        currentPosition += rayDir * push.stepSize;

        // Terminate if the ray exits the volume
        if (any(greaterThan(currentPosition, vec3(1.0))) || any(lessThan(currentPosition, vec3(0.0)))) {
            break;
        }
    }
    return vec4(0.0); // Return transparent if no hit
}

void main() {
    // Calculate ray origin (camera position) and ray direction
    vec3 rayOrigin = ubo.cameraPos.xyz;
    vec3 rayDirection = computeRayDirection(inUv);

    // Perform raymarching
    vec4 volumeColor = raymarch(rayOrigin, rayDirection);

    // Set the fragment color
    outColor = volumeColor;
}
