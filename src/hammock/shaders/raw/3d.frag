#version 450

// Inputs
layout (location = 0) in vec2 inUv;

// Outputs
layout (location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 projMatrix;
    mat4 viewMatrix;
    vec4 cameraPos;
    vec4 lightDirection;
    vec4 lightColor;
} ubo;

layout(binding = 1) uniform sampler3D volumeTexture;

// Push constants
layout (push_constant) uniform PushConstants {
    float stepSize;
    int maxSteps;
    float minDensityThreshold;
    float time;
} push;

// Converts clip space coordinates to view space ray direction
vec3 getRayDir(vec2 coord) {
    vec4 target = inverse(ubo.projMatrix) * vec4(coord, 1.0, 1.0);
    return normalize(vec3(inverse(ubo.viewMatrix) * vec4(target.xyz / target.w, 0.0)));
}

// Intersect ray with a unit cube centered at the origin
// Returns entry and exit distances along the ray
vec2 intersectCube(vec3 rayOrigin, vec3 rayDir) {
    vec3 tMin = (-1.0 - rayOrigin) / rayDir;
    vec3 tMax = (1.0 - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

// Sample volume with trilinear filtering
float sampleVolume(vec3 pos) {
    // Convert from [-1,1] to [0,1] range
    pos = pos * 0.5 + 0.5;
    return texture(volumeTexture, pos).r;
}

void main() {
    // Convert fragment coordinates to [-1,1] range
    vec2 coord = inUv * 2.0 - 1.0;

    // Get ray direction in world space
    vec3 rayDir = getRayDir(coord);
    vec3 rayOrigin = ubo.cameraPos.xyz;

    // Intersect with volume bounds
    vec2 bounds = intersectCube(rayOrigin, rayDir);
    if(bounds.x > bounds.y) {
        outColor = vec4(0.0);
        return;
    }

    // Clamp to near plane
    bounds.x = max(bounds.x, 0.0);

    // Initialize ray march variables
    vec3 pos = rayOrigin + bounds.x * rayDir;
    float transmittance = 1.0;
    vec3 color = vec3(0.0);

    // Ray march through volume
    for(int i = 0; i < push.maxSteps && bounds.x + i * push.stepSize < bounds.y; i++) {
        // Sample volume
        float density = sampleVolume(pos);

        if(density > push.minDensityThreshold) {
            // Simple diffuse lighting
            float shadow = sampleVolume(pos + ubo.lightDirection.xyz * push.stepSize);
            float lighting = max(0.1, (1.0 - shadow) * max(0.0, dot(ubo.lightDirection.xyz, -rayDir)));

            // Accumulate color and opacity
            vec3 sampleColor = ubo.lightColor.xyz * density * lighting;
            float alpha = density * push.stepSize;
            color += sampleColor * transmittance * alpha;
            transmittance *= 1.0 - alpha;

            // Early exit if nearly opaque
            if(transmittance < 0.01) break;
        }

        pos += rayDir * push.stepSize;
    }

    // Output final color with background blend
    vec3 backgroundColor = vec3(0.1, 0.2, 0.3);
    color += backgroundColor * transmittance;
    outColor = vec4(color, 1.0);
}
