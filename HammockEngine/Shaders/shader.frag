#version 450

// inputs
layout (location = 0) in vec3 fragColor;

// outputs
layout (location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(fragColor, 1.0);
}