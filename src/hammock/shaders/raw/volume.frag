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
    float density;
    float absorption;
    float phase;
    float stepSize;
    int maxSteps;
    float lMul;
    int maxLightSteps;
} params;

layout (set = 0, binding = 2) uniform sampler3D noiseTex;
layout (set = 0, binding = 3) uniform sampler3D signedDistanceField;

layout (push_constant) uniform PushConstants {
    mat4 cloudTransform;
} pushConstants;


// Sample signed distance filed
float sdf(vec3 p) {
    return texture(signedDistanceField, p * 0.5 + 0.5).r;
}

// Sample cloud density at a point
float sampleCloud(vec3 pos) {
    float dist = sdf(pos);

    if (dist > 0.0) return 0.0;

    // Sample noise for cloud detail
    float baseNoise = texture(noiseTex, pos * 0.1).r;
    float detailNoise = texture(noiseTex, pos * 0.3).r;

    return max(0.0, baseNoise * 0.625 + detailNoise * 0.375) * params.density;
}

// Light march through cloud
float lightMarch(vec3 pos) {
    float transmittance = 1.0;
    float stepSize = params.stepSize * params.lMul;  // Larger steps for light

    for (int i = 0; i < params.maxLightSteps; i++) {
        // Fewer steps for light
        pos += params.lightDir.xyz * stepSize;
        float density = sampleCloud(pos);
        transmittance *= exp(-density * stepSize * params.absorption);
    }

    return transmittance;
}

// Henyey-Greenstein phase function
float phaseFunction(float cosTheta) {
    float g = params.phase;
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.14159 * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}


// Compute surface normal using SDF gradient approximation
vec3 normal(vec3 p) {
    float epsilon = 0.001;
    return normalize(vec3(sdf(p + vec3(epsilon, 0.0, 0.0)) - sdf(p - vec3(epsilon, 0.0, 0.0)), sdf(p + vec3(0.0, epsilon, 0.0)) - sdf(p - vec3(0.0, epsilon, 0.0)), sdf(p + vec3(0.0, 0.0, epsilon)) - sdf(p - vec3(0.0, 0.0, epsilon))));
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


    float t = sdf(rayOrigin);
    vec3 pos = rayOrigin + rayDir * t;
    float transmittance = 1.0;
    vec3 scatteredLight = vec3(0.0);

    // Main raymarching loop
    for(int i = 0; i < params.maxSteps; i++) {

        float density = sampleCloud(pos);

        if(density > 0.0) {
            // Calculate lighting
            float light = lightMarch(pos);
            float cosTheta = dot(rayDir, params.lightDir.xyz);
            float phase = phaseFunction(cosTheta);

            // Accumulate scattered light
            vec3 luminance = params.lightColor.xyz * light  * density;
            scatteredLight += luminance * transmittance * params.stepSize;

            // Update transmittance
            transmittance *= exp(-density * params.stepSize * params.absorption);

            // Early exit if nearly opaque
            if(transmittance < 0.01) break;
        }

        t += params.stepSize;
        pos = rayOrigin + rayDir * t;
    }

    // Final color
    outColor = vec4(scatteredLight, 1.0 - transmittance);
}