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
    vec4 baseNoiseOffset;
    vec4 detailNoiseOffset;
    float baseNoiseScale;
    float detailNoiseScale;
    float noiseFactor;

    int numSteps;
    int numLightSteps;

    float baseDensityThreshold;
    float detailDensityThreshold;
    float density;
    float absorption;
    float scattering;
} params;

layout (set = 1, binding = 1) uniform sampler3D noiseTex;

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

float sampleDensity(vec3 position) {
    vec3 baseUvw = position * params.baseNoiseScale + params.baseNoiseOffset.xyz * 0.01;
    float baseNoise = texture(noiseTex, baseUvw).r;

    vec3 detailUvw = position * params.detailNoiseScale + params.detailNoiseOffset.xyz * 0.01;
    float detailNoise = texture(noiseTex, detailUvw).g;

    float shape = max(0.0, (1.0 - baseNoise) - params.baseDensityThreshold);
    float detail = max(0.0, (1.0 - baseNoise) - params.detailDensityThreshold);

    return max(0.0, shape * params.noiseFactor + detail * (1.0 - params.noiseFactor)) * params.density;
}


void main() {
    vec3 rayOrigin = getRayOrigin();
    vec3 rayDirection = getRayDirection();

    vec2 bboxIntersection = rayBoxDst(push.bb1.xyz, push.bb2.xyz, rayOrigin, rayDirection);
    float dstToBox = bboxIntersection.x;
    float dstInsideBox = bboxIntersection.y;

    // Ray hit
    if (dstInsideBox > 0) {

        if(dstInsideBox < 0.15){
            outColor = vec4(1.0, 0.0,0.0,1.0);
            return;
        }

        vec3 hitPos = rayOrigin + rayDirection * dstToBox;
        float depth = dstToBox;
        viewSpaceDepth = depth;

        float dstTravelled = 0.0;
        float stepSize = dstInsideBox / (params.numSteps < 1 ? 1 : params.numSteps);
        float dstLimit = dstInsideBox;

        float totalDensity = 0.0;
        while (dstTravelled < dstLimit) {
            vec3 rayPos = rayOrigin + rayDirection * (dstToBox + dstTravelled);
            totalDensity += sampleDensity(rayPos) * stepSize;
            dstTravelled += stepSize;
        }

        float transmittance = exp(-totalDensity);
        outColor = vec4(1.0-transmittance);
    }
    else {
        discard;
    }
}