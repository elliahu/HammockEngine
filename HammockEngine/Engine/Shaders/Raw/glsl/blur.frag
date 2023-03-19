#version 450

//inputs
layout (location = 0) in vec2 uv;

// outputs
layout (location = 0) out float outFragColor;

struct PointLight
{
    vec4 position;
    vec4 color;
    vec4 terms;
};

struct DirectionalLight
{
    vec4 direction;
    vec4 color;
};

// UBO
layout (set = 0, binding = 0) uniform GlobalUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    mat4 depthBiasMVP;
    vec4 ambientLightColor; // w is intensity
    DirectionalLight directionalLight;
    PointLight pointLights[10];
    int numLights;
} ubo;

layout (set = 1, binding = 0) uniform sampler2D samplerSSAO;

void main() 
{
	const int blurRange = 2;
	int n = 0;
	vec2 texelSize = 1.0 / vec2(textureSize(samplerSSAO, 0));
	float result = 0.0;
	for (int x = -blurRange; x < blurRange; x++) 
	{
		for (int y = -blurRange; y < blurRange; y++) 
		{
			vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(samplerSSAO, uv + offset).r;
			n++;
		}
	}
	outFragColor = result / (float(n));
}