#version 450
layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec4 pos;
    int width;
    int height;
} camera;

layout (set = 0, binding = 1) uniform CloudParams {
    vec4 lightDir;
    vec4 lightColor;
    vec4 baseSkyColor;
    vec4 gradientSkyColor;
    float stepSize;
    int maxSteps;
    float lMul;
    int maxLightSteps;
    float elapsedTime;
} params;

layout (set = 0, binding = 2) uniform sampler3D noiseTex;
layout (set = 0, binding = 3) uniform sampler3D signedDistanceField;

layout (push_constant) uniform PushConstants {
    mat4 cloudTransform;
    float density;
    float absorption;
    float scatteringAniso;
} pushConstants;

#define PI 3.14159265359

// Transform a point from world space to cloud space
vec3 worldToCloudSpace(vec3 worldPos) {
    vec4 cloudPos = inverse(pushConstants.cloudTransform) * vec4(worldPos, 1.0);
    return cloudPos.xyz;
}

float beer(float dist, float absorption) {
    return exp(-dist * absorption);
}

float henyeyGreenstein(float g, float mu) {
    float gg = g * g;
    return (1.0 / (4.0 * PI)) * ((1.0 - gg) / pow(1.0 + gg - 2.0 * g * mu, 1.5));
}


// Sample signed distance field
float sdf(vec3 p) {
    // Transform the sampling position to cloud space
    vec3 cloudSpacePos = worldToCloudSpace(p);
    return texture(signedDistanceField, cloudSpacePos * 0.5 + 0.5).r;
}

// Sample cloud density at a point
float sampleCloud(vec3 pos) {
    float dist = sdf(pos);

    if (dist > 0.0) return 0.0;

    // Transform the sampling position to cloud space for noise sampling
    vec3 cloudSpacePos = worldToCloudSpace(pos);

    // Sample noise for cloud detail
    float baseNoise = texture(noiseTex, cloudSpacePos * 0.1).r;
    float detailNoise = texture(noiseTex, cloudSpacePos * 0.3).r;

    return max(0.0, baseNoise * 0.625 + detailNoise * 0.375) * pushConstants.density;
}

// Compute surface normal using SDF gradient approximation
vec3 normal(vec3 p) {
    float epsilon = 0.001;
    return normalize(vec3(sdf(p + vec3(epsilon, 0.0, 0.0)) - sdf(p - vec3(epsilon, 0.0, 0.0)), sdf(p + vec3(0.0, epsilon, 0.0)) - sdf(p - vec3(0.0, epsilon, 0.0)), sdf(p + vec3(0.0, 0.0, epsilon)) - sdf(p - vec3(0.0, 0.0, epsilon))));
}


float lightmarch(vec3 position, vec3 rayDirection) {
    float totalDensity = 0.0;
    float marchSize = 0.03;

    for (int step = 0; step < params.maxLightSteps; step++) {
        position += normalize(params.lightDir.xyz) * marchSize * float(step) * params.lMul;

        float lightSample = sampleCloud(position);
        totalDensity += lightSample;
    }

    float transmittance = beer(totalDensity, pushConstants.absorption);
    return transmittance;
}

float raymarch(vec3 rayOrigin, vec3 rayDirection, float offset) {
    float depth = 0.0;
    depth += params.stepSize * offset;
    vec3 p = rayOrigin + depth * rayDirection;

    float totalTransmittance = 1.0;
    float lightEnergy = 0.0;

    float phase = henyeyGreenstein(pushConstants.scatteringAniso, dot(rayDirection, normalize(params.lightDir.xyz)));

    for (int i = 0; i < params.maxSteps; i++) {
        float density = sampleCloud(p);

        // We only draw the transmittance if it's greater than 0
        if (density > 0.0) {
            float lightTransmittance = lightmarch(p, rayDirection);
            float luminance = 0.025 + density * phase;

            totalTransmittance *= lightTransmittance;
            lightEnergy += totalTransmittance * luminance;
        }

        depth += params.stepSize;
        p = rayOrigin + depth * rayDirection;
    }

    return lightEnergy;
}

void main() {
    mat4 inverseView = inverse(camera.view);

    // Calculate aspect ratio
    float aspectRatio = float(camera.width) / float(camera.height);

    vec2 uv = gl_FragCoord.xy / vec2(camera.width, camera.height);
    uv -= 0.5;
    uv.x *= aspectRatio;
    uv.y *= -1.0;

    // Ray Origin - camera
    vec3 rayOrigin = camera.pos.xyz;

    // Ray Direction
    vec3 rayDir = normalize(vec3(uv, -1.0));

    // Transform ray direction by the camera's orientation
    rayDir = (inverseView * vec4(rayDir, 0.0)).xyz;


    vec3 color = vec3(0.0);

    // Sun and Sky
    vec3 sunColor = params.lightColor.rgb;
    vec3 sunDirection = normalize(params.lightDir.xyz);
    sunDirection.y *= -1;
    float sun = clamp(dot(sunDirection, rayDir), 0.0, 1.0);
    // Base sky color
    color = params.baseSkyColor.rgb;
    // Add vertical gradient
    color -= params.gradientSkyColor.a* params.gradientSkyColor.rgb * -rayDir.y;
    // Add sun color to sky
    color += 0.5 * sunColor * pow(sun, 10.0);

//    float blueNoise = texture(blueNoiseSampler, gl_FragCoord.xy / 1024.0).r;
//    float offset = fract(blueNoise);

    // Cloud
    float res = raymarch(rayOrigin, rayDir, 0);
    color = color + sunColor * res;

    outColor = vec4(color, 1.0);
}