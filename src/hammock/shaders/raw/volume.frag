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
    float cloudsMin;
    float cloudsMax;
    float noiseScale;
    float noiseLowerCutoff;
    float noiseHigherCutoff;
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

// Improved light extinction calculation
float beer(float density, float rayDist) {
    // Beer-Lambert law with improved extinction coefficient
    return exp(-density * rayDist * pushConstants.absorption);
}

// Multiple scattering approximation from the paper
float multipleScattering(float density) {
    // Powder effect approximation
    float powder = 1.0 - exp(-density * 2.0);
    return mix(1.0, powder, 0.5); // Blend between single and multiple scattering
}


// Improved phase function (Section 2.2 of the paper)
float henyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}


// Improved phase function with dual-lobe scattering
float dualLobePhase(float cosTheta) {
    float back = henyeyGreenstein(cosTheta, -0.3); // Back-scattering lobe
    float forward = henyeyGreenstein(cosTheta, pushConstants.scatteringAniso); // Forward-scattering lobe
    return mix(forward, back, 0.7); // Blend between forward and backward scattering
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
    float stepSize = 0.03;
    float transmittance = 1.0;

    for (int step = 0; step < params.maxLightSteps; step++) {
        position += normalize(params.lightDir.xyz) * stepSize * float(step) * params.lMul;
        float lightSample = sampleCloud(position);

        // Accumulate density with improved extinction
        if (lightSample > 0.0) {
            totalDensity += lightSample;
            // Early exit for performance
            if (transmittance < 0.01) break;
        }
    }

    return beer(totalDensity, stepSize * params.maxLightSteps);
}

float raymarch(vec3 rayOrigin, vec3 rayDirection, float offset) {
    float depth = 0.0;
    depth += params.stepSize * offset;
    vec3 p = rayOrigin + depth * rayDirection;

    float transmittance = 1.0;
    float lightEnergy = 0.0;

    // Calculate cosine of angle between view and light directions
    float cosTheta = dot(rayDirection, normalize(params.lightDir.xyz));

    // Calculate phase function value
    float phase = dualLobePhase(cosTheta);

    for (int i = 0; i < params.maxSteps; i++) {
        float density = sampleCloud(p);

        if (density > 0.0) {
            // Calculate light contribution
            float lightTransmittance = lightmarch(p, rayDirection);

            // Multiple scattering approximation
            float ms = multipleScattering(density);

            // Combine all lighting factors
            float luminance = density * phase * ms;

            // Accumulate light energy with transmittance
            lightEnergy += transmittance * luminance * lightTransmittance;

            // Update transmittance along view ray
            transmittance *= beer(density, params.stepSize);

            // Early exit if transmittance is too low
            if (transmittance < 0.01) break;
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
    color += 0.3 * sunColor * pow(sun, 10.0);

//    float blueNoise = texture(blueNoiseSampler, gl_FragCoord.xy / 1024.0).r;
//    float offset = fract(blueNoise);

    // Cloud
    float res = raymarch(rayOrigin, rayDir, 0);
    color = color + (sunColor * res);

    outColor = vec4(color, 1.0);
}