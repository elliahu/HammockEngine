#version 450

layout (set = 0, binding = 2) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outFragColor;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outMaterial;

void main() 
{
	vec3 color = pow(texture(samplerCubeMap, inUVW).rgb, vec3(1.5));
	outFragColor = vec4(color, 1.0);

	outPosition = vec4(inUVW,1);
	outNormal = vec4(0);
	outMaterial = vec4(-1);
}