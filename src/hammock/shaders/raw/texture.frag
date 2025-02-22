#version 450

layout (binding = 0) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;


layout (push_constant) uniform Constants {
    vec4 lightPos;   // Light position in world space
    vec4 lightColor; // Light color
    vec4 bbmin;      // Bounding box min (world space)
    vec4 bbmax;      // Bounding box max (world space)
    vec4 cameraPos;  // Camera position
    mat4 viewProj;   // View-projection matrix
} push;



void main()
{
    vec4 skyColor = vec4(vec3(0.4627, 0.6549, 0.9568),1.0);
    vec4 cloudsColor = texture(samplerColor, vec2(inUV.s, 1.0 - inUV.t));

    outFragColor = mix(skyColor, cloudsColor, cloudsColor.a);
}