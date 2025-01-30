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
} params;

layout (set = 0, binding = 2) uniform sampler3D noiseTex;
layout (set = 0, binding = 3) uniform sampler3D signedDistanceField;

layout (push_constant) uniform PushConstants {
    mat4 cloudTransform;
} pushConstants;


float sdf(vec3 p){
    return texture(signedDistanceField, p* 0.5 + 0.5).r;
}


// Compute surface normal using SDF gradient approximation
vec3 getNormal(vec3 p) {
    float epsilon = 0.001;
    return normalize(vec3(
                     sdf(p + vec3(epsilon, 0.0, 0.0)) - sdf(p - vec3(epsilon, 0.0, 0.0)),
                     sdf(p + vec3(0.0, epsilon, 0.0)) - sdf(p - vec3(0.0, epsilon, 0.0)),
                     sdf(p + vec3(0.0, 0.0, epsilon)) - sdf(p - vec3(0.0, 0.0, epsilon))
                     ));
}


void main() {
    mat4 inverseView = inverse(camera.view);

    // Calculate aspect ratio
    float aspectRatio = float(camera.width) / float(camera.height);

    vec2 uv = gl_FragCoord.xy / vec2(camera.width, camera.height);
    uv -= 0.5;
    uv.x *= float(camera.width) / float(camera.height);


    // Flip the Y-coordinate for Vulkan
    uv.y *= -1.0;

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
        // Compute the distance to the closest surface (Torus SDF)
        float dist = sdf(pos);  // Torus with radii 1.0 and 0.5

        if (dist < 0.01) {
            // If we hit something, compute the normal
            vec3 normal = getNormal(pos);

            // Lighting calculations
            // 1. Ambient lighting
            vec3 ambient = 0.1 * params.lightColor.rgb;  // Basic ambient light

            // 2. Diffuse lighting (Lambertian)
            vec3 lightDir = normalize(params.lightDir.xyz);
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * params.lightColor.rgb;

            // 3. Specular lighting (Phong)
            vec3 viewDir = normalize(rayOrigin - pos);
            vec3 reflectDir = reflect(-lightDir, normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);  // Hardcoded shininess
            vec3 specular = 0.5 * spec * params.lightColor.rgb;  // Specular strength of 0.5

            // Combine lighting components
            vec3 color = ambient + diffuse + specular;

            // Output color
            outColor = vec4(color, 1.0);
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