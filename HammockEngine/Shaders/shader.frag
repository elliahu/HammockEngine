#version 450

//inputs
layout (location = 0) in vec3 fragColor;

// outputs
layout (location = 0) out vec4 outColor;


// push constants
layout (push_constant) uniform Push
{
    mat4 modelMatrix; // model matrix
    mat4 normalMatrix; // using mat4 bcs alignment requirements
} push;


void main()
{
	outColor = vec4(fragColor, 1.0);
}