#version 450
layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;
layout (location = 1) out float viewSpaceDepth;

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

layout (set = 1, binding = 0) uniform CloudParams {
    vec4 lowFreqWeights;
    vec4 highFreqWeights;
    vec4 freqSampleOffset;
    float lowFreqScale;
    float highFreqScale;
    float freqSampleScale;
    float freqOffset;
    float densityMultiplier;

    int numSteps;
    int numLightSteps;

    float phaseG;
} params;

layout (set = 1, binding = 1) uniform sampler3D lowFreqNoiseSampler;
layout (set = 1, binding = 2) uniform sampler3D highFreqNoiseSampler;

layout (push_constant) uniform PushConstants {
    vec4 bb1;
    vec4 bb2;
} push;

#define PI 3.14159265359

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

// Returns x = distance from camera to the first intersection, y = distance from the first intersection to the second intersection
vec2 rayBoxDst(vec3 boundMin, vec3 boundMax, vec3 rayOrigin, vec3 rayDirection) {
    vec3 t0 = (boundMin - rayOrigin) / rayDirection;
    vec3 t1 = (boundMax - rayOrigin) / rayDirection;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    float dstA = max(max(tmin.x, tmin.y), tmin.z);
    float dstB = min(tmax.x, min(tmax.y, tmax.z));

    float dstToBox = max(0, dstA);
    float dstInsideBox = max(0, dstB - dstToBox);
    return vec2(dstToBox, dstInsideBox);
}

float remap(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

float saturate(float value) {
    return clamp(value, 0.0, 1.0);
}

float getLowFreqNoise(vec3 samplePosition){
    vec4 lowFeqNoiseRGBA = texture(lowFreqNoiseSampler, samplePosition);
    vec4 lowFreqChannelWeights = normalize(params.lowFreqWeights);
    float lowFreqDot = dot(lowFeqNoiseRGBA, lowFreqChannelWeights);
    float lowFreqNoise = lowFreqDot + params.freqOffset;
    return lowFreqNoise * params.lowFreqScale;
}

float getHighFreqNoise(vec3 samplePosition){
    vec3 highFreqNoiseRGB = texture(highFreqNoiseSampler, samplePosition).rgb;
    vec3 highFreqChannelWeights = normalize(params.highFreqWeights.rgb);
    float highFreqDot = dot(highFreqNoiseRGB, highFreqChannelWeights);
    float highFreqNoise = highFreqDot + params.freqOffset;
    return highFreqNoise * params.highFreqScale;
}

float sampleDensity(vec3 position) {
    vec3 size = push.bb1.xyz - push.bb2.xyz;
    vec3 uvw = position * params.freqSampleScale;
    vec3 samplePosition = uvw + params.freqSampleOffset.xyz;

    float lowFreqNoise = getLowFreqNoise(samplePosition);

    if(lowFreqNoise > 0.0){
        float highFreqNoise = getHighFreqNoise(samplePosition);
        float baseCloudDensity = remap(lowFreqNoise, highFreqNoise, 1.0, 0.0, 1.0);
        return baseCloudDensity * params.densityMultiplier;
    }

    return 0.0;
}

float attenuation(float densityAlongRay){
    return exp(-densityAlongRay);
}

float henyeyGreenstein(float cosTheta, float g){
    return (1.0 / 4.0 * PI) * ((1 - (g * g)) / pow((1 - (g * cosTheta)), 2));
}

float multipleScattering(float densitySample){
    return 1 - exp(-densitySample);
}


float lightmarch(vec3 position) {
    vec3 rayDirection = -normalize(camera.lightDir.xyz);
    float dstInsideBox = rayBoxDst(push.bb1.xyz, push.bb2.xyz, position, rayDirection).y;

    if (dstInsideBox <= 0.0) {
        return 0.0; // No light interaction if outside the bounding box
    }

    float stepSize = dstInsideBox / float(params.numLightSteps);
    float totalDensity = 0.0;
    float totalLight = 0.0;

    for (int step = 0; step < params.numLightSteps; step++) {
        float dstTraveled = step * stepSize;
        vec3 rayPos = position + rayDirection * dstTraveled;
        float densitySample = sampleDensity(rayPos) * stepSize;

        totalDensity += densitySample;
        float transmittance = exp(-totalDensity);
        float phase = henyeyGreenstein(dot(rayDirection, normalize(camera.lightDir.xyz)), params.phaseG);

        totalLight += transmittance * phase * multipleScattering(densitySample);
    }

    return totalLight;
}


void main() {
    vec3 rayOrigin = getRayOrigin();
    vec3 rayDirection = getRayDirection();

    vec2 bboxIntersection = rayBoxDst(push.bb1.xyz, push.bb2.xyz, rayOrigin, rayDirection);
    float dstToBox = bboxIntersection.x;
    float dstInsideBox = bboxIntersection.y;

    if (dstInsideBox > 0) {
        if (dstInsideBox < 0.15) {
            outColor = vec4(1.0, 0.0, 0.0, 1.0); // Debug: Too thin to render
            return;
        }

        vec3 hitPos = rayOrigin + rayDirection * dstToBox;
        viewSpaceDepth = dstToBox;

        float dstTravelled = 0.0;
        float stepSize = dstInsideBox / float(params.numSteps);
        float dstLimit = dstInsideBox;

        float totalDensity = 0.0;
        vec3 accumulatedLight = vec3(0.0);
        float alpha = 0.0;  // Track accumulated opacity

        while (dstTravelled < dstLimit) {
            vec3 rayPos = rayOrigin + rayDirection * (dstToBox + dstTravelled);
            float densitySample = sampleDensity(rayPos) * stepSize;
            totalDensity += densitySample;

            float transmittance = exp(-totalDensity); // Beer's Law
            float lightEnergy = lightmarch(rayPos);
            vec3 scatteredLight = lightEnergy * camera.lightColor.rgb;

            accumulatedLight += scatteredLight * transmittance * densitySample;

            // Compute alpha accumulation for blending
            alpha += (1.0 - alpha) * densitySample * stepSize;

            dstTravelled += stepSize;
        }

        vec3 finalColor = accumulatedLight;
        outColor = vec4(finalColor, alpha);  // Alpha included for blending
    }
    else {
        discard;
    }
}