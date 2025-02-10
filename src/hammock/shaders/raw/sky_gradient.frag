#version 450

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec4 pos;
    vec4 lightDir;
    vec4 lightColor;
    vec4 baseSkyColor;
    vec4 gradientSkyColor;
    vec4 groundColor;
    int width;
    int height;
    float sunFactor;
    float sunExp;
} camera;


vec3 getRayOrigin() {
    return camera.pos.xyz;
}

vec3 getRayDirection() {
    mat4 inverseView = inverse(camera.view);

    // Calculate aspect ratio
    float aspectRatio = float(camera.width) / float(camera.height);

    vec2 uv = gl_FragCoord.xy / vec2(camera.width, camera.height);
    uv -= 0.5;
    uv.x *= aspectRatio;
    uv.y *= -1.0;

    // Ray Direction
    vec3 rayDir = normalize(vec3(uv, -1.0));

    // Transform ray direction by the camera's orientation
    rayDir = (inverseView * vec4(rayDir, 0.0)).xyz;

    return rayDir;
}

void main() {
    vec3 rayDir = getRayDirection();
    vec3 sunColor = camera.lightColor.rgb;
    vec3 sunDirection = normalize(camera.lightDir.xyz);

    float sun = clamp(dot(sunDirection, rayDir), 0.0, 1.0);

    // Gradient factor: -1 (down) → 0 (horizon) → 1 (up)
    float gradientFactor = clamp(rayDir.y * 0.5 + 0.5, 0.0, 1.0);

    // Blend from ground → horizon → sky
    vec3 color = mix(camera.groundColor.rgb, camera.gradientSkyColor.rgb, smoothstep(0.0, 0.5, gradientFactor));
    color = mix(color, camera.baseSkyColor.rgb, smoothstep(0.5, 1.0, gradientFactor));

    // Add sun effect
    color += camera.sunFactor * sunColor * pow(sun, camera.sunExp);

    outColor = vec4(color, 1.0);
}