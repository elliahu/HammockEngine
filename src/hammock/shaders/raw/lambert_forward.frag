#version 450

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 position;
layout (location = 3) in vec4 tangent;

layout (location = 0) out vec4 color;


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



void main() {
    // Normalize the normal vector
    vec3 norm = normalize(normal);

    // Compute the light direction
    vec3 lightDir = normalize(camera.lightDir.xyz);

    // Ambient lighting (basic white light)
    vec3 ambient = vec3(0.1);

    // Diffuse lighting (Lambert's cosine law)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * camera.lightColor.rgb; // white diffuse color

    // Specular lighting (Phong model)
    vec3 viewDir = normalize(-position); // Simple view direction assumption
    vec3 reflectDir = reflect(-lightDir, norm);

    // Combine the ambient, diffuse, and specular components
    vec3 finalColor = ambient + diffuse;

    // Output the final color
    color = vec4(finalColor, 1.0);
}