#version 450

layout (binding = 0) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec4 skyColor = vec4(vec3(0.4627, 0.6549, 0.9568),1.0);
    vec4 cloudsColor = texture(samplerColor, vec2(inUV.s, 1.0 - inUV.t));

    outFragColor = mix(skyColor, cloudsColor, cloudsColor.a);
}