#version 450
layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec4 pos;
    int width;
    int height;
} camera;

layout (set = 0, binding = 1) uniform CloudParams {
    vec4 lightDir;
    vec4 lightColor;
    float density;
    float absorption;
    float phase;
    float stepSize;
    int maxSteps;
} params;

layout (set = 0, binding = 2) uniform sampler3D noiseTex;

layout (push_constant) uniform PushConstants {
    mat4 cloudTransform;
} pushConstants;

float sdTorus( vec3 p, vec2 t )
{
    vec2 q = vec2(length(p.xz)-t.x,p.y);
    return length(q)-t.y;
}

void main() {
    mat4 inverseView = inverse(camera.view);

    // Calculate aspect ratio
    float aspectRatio = float(camera.width) / float(camera.height);

    vec2 uv = gl_FragCoord.xy / vec2(camera.width, camera.height);
    uv -= 0.5;
    uv.x *= float(camera.width) / float(camera.height);

    // Flip the Y-coordinate for Vulkan
    //uv.y *= -1.0;

    // Ray Origin - camera
    vec3 rayOrigin = camera.pos.xyz;

    // Ray Direction
    vec3 rayDir = normalize(vec3(uv, -1.0));

    // Transform ray direction by the camera's orientation
    rayDir = (inverseView * vec4(rayDir, 0.0)).xyz;


    float t = 0.0;  // Ray marching distance
    vec3 pos = rayOrigin + rayDir * t;

    // Raymarching loop
    for (int i = 0; i < params.maxSteps; i++) {
        // Compute the distance to the closest surface (simple sphere SDF)
        float dist = sdTorus(pos, vec2(1.0 , 0.5));  // Sphere with radius 0.2

        if (dist < 0.01) {
            // If we hit something, output color
            outColor = vec4(1.0, 0.0, 0.0, 1.0);  // Red color
            return;
        }

        // Accumulate ray distance
        t += dist * 0.5;  // Step size is half of the distance
        pos = rayOrigin + rayDir * t;

        // Early exit if the ray is too far
        if (t > 10.0) {
            outColor = vec4(0.0, 0.0, 0.0, 1.0);  // Black background
            return;
        }
    }

    // If no hit is found, output background color
    outColor = vec4(0.5, 0.5, 0.5, 1.0);  // Gray background
}