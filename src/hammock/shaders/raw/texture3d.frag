#version 450

layout (binding = 0) uniform sampler3D samplerColor;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants {
    vec4 displayChannels;
    float slice;
    float scale;
} push;


void main()
{
    vec4 color = texture(samplerColor, vec3(inUV.s, 1.0 - inUV.t, push.slice) * push.scale);

    if(push.displayChannels.x > 0.0){
        outFragColor.x = color.x;
    }
    else{
        outFragColor.x = 0.0;
    }
    if(push.displayChannels.y > 0.0){
        outFragColor.y = color.y;
    }
    else{
        outFragColor.y = 0.0;
    }

    if(push.displayChannels.z > 0.0){
        outFragColor.z = color.z;
    }
    else{
        outFragColor.z = 0.0;
    }

    if(push.displayChannels.w > 0.0){
        outFragColor.w = color.w;
    }
    else{
        outFragColor.w = 1.0;
    }
}