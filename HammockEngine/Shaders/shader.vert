#version 450

// inputs
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;

//outputs

// push constants
layout (push_constant) uniform Push
{
    mat2 transform;
    vec2 offset;
    vec3 color;
} push;

void main() 
{
    gl_Position = vec4(push.transform * position + push.offset, 0.0, 1.0);
}