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

layout (set = 0, binding = 1) uniform AtmosphereParams {
    vec4 lightDir;
    vec4 lightColor;
    vec4 baseSkyColor;
    vec4 gradientSkyColor;
    vec4 rayleighScattering;
    vec4 mieScattering;
    float stepSize;
    int maxSteps;
    float lMul;
    int maxLightSteps;
    float elapsedTime;
    float atmosphereHeight;
    float planetRadius;
    float mieG;
} params;

layout (set = 0, binding = 2) uniform sampler3D noiseTex;
layout (set = 0, binding = 3) uniform sampler3D signedDistanceField;
layout (set = 0, binding = 4) uniform sampler2D weatherMap;

layout (push_constant) uniform PushConstants {
    mat4 cloudTransform;
    float density;
    float absorption;
    float scatteringAniso;
    float cloudBaseHeight;
    float cloudTopHeight;
} pushConstants;

#define PI 3.14159265359

// Atmosphere intersection test
bool intersectAtmosphere(vec3 rayOrigin, vec3 rayDir, out float near, out float far) {
    float R = params.planetRadius;
    float H = params.atmosphereHeight;

    vec3 center = vec3(0, -R, 0); // Planet center
    float atmR = R + H; // Atmosphere radius

    vec3 L = rayOrigin - center;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayDir, L);
    float c = dot(L, L) - atmR * atmR;

    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0) return false;

    near = (-b - sqrt(discriminant)) / (2.0 * a);
    far = (-b + sqrt(discriminant)) / (2.0 * a);

    return true;
}

// Improved Henyey-Greenstein phase function
float henyeyGreenstein(float g, float mu) {
    float gg = g * g;
    return (1.0 - gg) / (4.0 * PI * pow(1.0 + gg - 2.0 * g * mu, 1.5));
}

// Weather map sampling
vec4 sampleWeather(vec3 pos) {
    vec2 uv = pos.xz * 0.001 + 0.5;
    return texture(weatherMap, uv);
}

// Improved cloud shape function
float getCloudShape(vec3 pos, vec3 cloudSpacePos) {
    // Height gradient
    float heightFraction = (pos.y - pushConstants.cloudBaseHeight) /
    (pushConstants.cloudTopHeight - pushConstants.cloudBaseHeight);
    heightFraction = clamp(heightFraction, 0.0, 1.0);

    // Anvil shape curve
    float cloudAnvil = 1.0 - pow(heightFraction, 0.4);

    // Weather influence
    vec4 weather = sampleWeather(pos);
    float coverage = weather.r;
    float cloudType = weather.g;

    // Noise sampling
    float baseFreq = 0.1;
    float baseNoise = texture(noiseTex, cloudSpacePos * baseFreq).r;
    float detail1 = texture(noiseTex, cloudSpacePos * baseFreq * 2.0).r * 0.5;
    float detail2 = texture(noiseTex, cloudSpacePos * baseFreq * 4.0).r * 0.25;

    float fbm = baseNoise + detail1 + detail2;

    // Combine factors
    float baseShape = fbm * cloudAnvil * coverage;
    float density = max(0.0, baseShape - (1.0 - cloudType) * 0.2);

    return density * pushConstants.density;
}

// Improved multiple scattering approximation
vec3 multipleScattering(float density, float mu) {
    float powder = 1.0 - exp(-density * 2.0);
    float backscatter = 0.3 * powder;

    float phase = henyeyGreenstein(pushConstants.scatteringAniso, mu);
    float forwardScatter = phase * (1.0 - powder);

    return vec3(backscatter + forwardScatter);
}

vec3 worldToCloudSpace(vec3 worldPos) {
    vec4 cloudPos = inverse(pushConstants.cloudTransform) * vec4(worldPos, 1.0);
    return cloudPos.xyz;
}

// Improved light marching with energy conservation
vec3 lightmarch(vec3 position, vec3 rayDirection) {
    float stepSize = params.stepSize * params.lMul;
    vec3 lightStep = normalize(params.lightDir.xyz) * stepSize;

    float transmittance = 1.0;
    vec3 totalScattering = vec3(0.0);

    for (int step = 0; step < params.maxLightSteps; step++) {
        position += lightStep;

        vec3 cloudSpacePos = worldToCloudSpace(position);
        float density = getCloudShape(position, cloudSpacePos);

        if (density > 0.0) {
            float extinction = density * pushConstants.absorption;
            transmittance *= exp(-extinction * stepSize);

            if (transmittance < 0.01) break;
        }
    }

    return vec3(transmittance);
}

vec3 raymarch(vec3 rayOrigin, vec3 rayDirection) {
    float near, far;
    if (!intersectAtmosphere(rayOrigin, rayDirection, near, far))
    return vec3(0.0);

    float depth = max(0.0, near);
    vec3 pos = rayOrigin + depth * rayDirection;

    vec3 totalTransmittance = vec3(1.0);
    vec3 totalScattering = vec3(0.0);

    float mu = dot(rayDirection, normalize(params.lightDir.xyz));

    for (int i = 0; i < params.maxSteps; i++) {
        vec3 cloudSpacePos = worldToCloudSpace(pos);
        float density = getCloudShape(pos, cloudSpacePos);

        if (density > 0.0) {
            vec3 lightContrib = lightmarch(pos, rayDirection);
            vec3 scattering = multipleScattering(density, mu);

            vec3 extinction = vec3(density * pushConstants.absorption);
            vec3 transmittance = exp(-extinction * params.stepSize);

            totalScattering += totalTransmittance * scattering * lightContrib * params.lightColor.rgb;
            totalTransmittance *= transmittance;

            if (all(lessThan(totalTransmittance, vec3(0.01))))
            break;
        }

        depth += params.stepSize;
        pos = rayOrigin + depth * rayDirection;

        if (depth > far) break;
    }

    return totalScattering;
}

void main() {
    // Calculate ray direction
    float aspectRatio = float(camera.width) / float(camera.height);
    vec2 ndc = (gl_FragCoord.xy / vec2(camera.width, camera.height)) * 2.0 - 1.0;
    ndc.x *= aspectRatio;
    ndc.y *= -1;

    vec3 rayDir = normalize(vec3(ndc, -1.0));
    rayDir = (inverse(camera.view) * vec4(rayDir, 0.0)).xyz;

    // Compute atmosphere and clouds
    vec3 cloudColor = raymarch(camera.pos.xyz, rayDir);

    // Sky background (simplified for this example)
    vec3 skyColor = params.baseSkyColor.rgb;

    // Sun
    float sun = max(0.0, dot(normalize(params.lightDir.xyz), rayDir));

    // Combine final color
    vec3 finalColor = skyColor + cloudColor;

    outColor = vec4(finalColor, 1.0);
}