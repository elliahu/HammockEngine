#version 450

//inputs
layout (location = 0) in vec2 uv;

// outputs
layout (location = 0) out vec4 outColor;

// gbuffer attachments  
layout(set = 4, binding = 0) uniform sampler2D positionSampler;
layout(set = 4, binding = 1) uniform sampler2D albedoSampler;
layout(set = 4, binding = 2) uniform sampler2D normalSampler;
layout(set = 4, binding = 3) uniform sampler2D materialPropertySampler;

#define SHADOW_FACTOR 0.0
#define SSAO_CLAMP .99
#define EXPOSURE 3.5
#define GAMMA .8

struct OmniLight
{
    vec4 position;
    vec4 color;
};


layout(set = 0, binding = 0) uniform Environment
{
    OmniLight omniLights[10];
    uint numOmniLights;
} env;

layout (set = 1, binding = 0) uniform SceneUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
} scene;

bool isSkybox(vec3 material)
{
	return material == vec3(-1);
}

void main()
{
    vec3 N = normalize(texture(normalSampler, uv).rgb);
	vec3 position = texture(positionSampler, uv).rgb;
	vec3 V = normalize(- position);
	vec3 albedo = texture(albedoSampler, uv).rgb;
	vec3 material = texture(materialPropertySampler, uv).rgb;
	float roughness = material.r;
	float metallic = material.g;
	float ao = material.b;
	vec3 ambient = albedo * 0.1;

	// Specular contribution
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < env.numOmniLights; i++) {
		vec3 L = normalize(env.omniLights[i].position.xyz - position);
		float diff = max(dot(N,L),0.0);
        vec3 diffuse = diff * env.omniLights[i].color.xyz;

        Lo += (diffuse + ambient) * albedo;
	};

	if(!isSkybox(material))
		outColor = vec4(Lo, 1.0);
	else 
		outColor = vec4(albedo, 1.0);
}