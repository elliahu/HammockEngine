#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (location = 0) out vec4 finalColor;

layout (set = 0, binding = 0) uniform SceneUbo {
    mat4 mvp;
} ubo;

void main(){
    // Static light position in world space
    vec3 lightPosition = vec3(10.0, 10.0, 10.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0); // White light

    // Material properties (hardcoded)
    vec3 ambientReflectance = vec3(0.1, 0.1, 0.1); // Ambient color
    vec3 diffuseReflectance = vec3(1.0, 1.0, 1.0); // Yellow diffuse
    vec3 specularReflectance = vec3(1.0, 1.0, 1.0); // White specular
    float shininess = 32.0;

    // Ambient lighting
    vec3 ambient = ambientReflectance * lightColor;

    // Diffuse lighting
    vec3 normal = normalize(normal);
    vec3 lightDir = normalize(lightPosition - position);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseReflectance * diff * lightColor;

    // Specular lighting
    vec3 viewDir = normalize(-vec3(0,0,6)); // Assuming the camera is at (0,0,0)
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularReflectance * spec * lightColor;

    // Combine components
    vec3 color = ambient + diffuse + specular;

    // Output final color
    finalColor = vec4(color, 1.0);
}