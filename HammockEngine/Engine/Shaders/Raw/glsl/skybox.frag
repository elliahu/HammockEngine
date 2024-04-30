#version 450

layout(set = 0, binding = 2) uniform sampler2D environmentSampler;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outFragColor;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outMaterial;

layout(set = 1, binding = 0) uniform SceneUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
} scene;

void main() 
{
    // Transform UV coordinates to a direction in view space
    vec4 direction_view = inverse(scene.projection * scene.view) * vec4(in_uv * 2.0 - 1.0, 1.0, 1.0);
    vec3 direction = normalize(direction_view.xyz / direction_view.w);
    
    // Convert 3D direction to 2D UV coordinates using equirectangular projection
    vec2 uv;
    uv.x = atan(direction.z, direction.x) / (2.0 * 3.14159265358979323846) + 0.5;
    uv.y = asin(direction.y) / 3.14159265358979323846 + 0.5;
    
    // Sample the environment map in the calculated direction
    vec3 environmentColor = texture(environmentSampler, uv).rgb;

    // Output the environment color as fragment color
    outFragColor = vec4(environmentColor, 1.0);

    outPosition = vec4(0);
    outNormal = vec4(0);
    outMaterial = vec4(-1);
}
