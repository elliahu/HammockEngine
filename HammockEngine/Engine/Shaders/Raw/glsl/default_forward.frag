#version 450
#extension GL_EXT_nonuniform_qualifier : enable

#define INVALID_TEXTURE 4294967295


// inputs
layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 position;
layout (location = 3) in vec4 tangent;

// outputs
layout (location = 0) out vec4 outColor;


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

layout(set = 0, binding = 1) uniform sampler2D textures[];

layout (set = 1, binding = 0) uniform SceneUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
} scene;



layout (set = 2, binding = 0) uniform TransformUbo
{
    mat4 model;
    mat4 normal;
} transform;

layout (set = 3, binding = 0) uniform MaterialPropertyUbo
{
    vec4 baseColorFactor;
    uint baseColorTextureIndex;
    uint normalTextureIndex;
    uint metallicRoughnessTextureIndex;
    uint occlusionTextureIndex;
    float alphaCutoff;
	float metallicFactor;
	float roughnessFactor;
} material;


const float PI = 3.14159265359;

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, float metallic, vec3 albedo)
{
	vec3 F0 = mix(vec3(0.04), albedo, metallic); // * material.specular
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); 
	return F;    
}

// Specular BRDF composition --------------------------------------------

vec3 BRDF(vec3 L, vec3 V, vec3 N,vec3 albedo, vec3 lightColor, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0)
	{
		float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = F_Schlick(dotNV, metallic,albedo);

		vec3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}

// Tonemap -----------------------------------------------------
#define EXPOSURE 4.5
#define GAMMA 1.0
vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
    vec3 N = normalize(normal);
	vec3 V = normalize(scene.inverseView[3].xyz - position);

	vec3 albedo;
	if(material.baseColorTextureIndex == INVALID_TEXTURE)
		albedo = material.baseColorFactor.rgb;
	else 
		albedo = pow(texture(textures[material.baseColorTextureIndex], uv).rgb, vec3(2.2));

	float roughness;
	if(material.metallicRoughnessTextureIndex == INVALID_TEXTURE)
		roughness = material.metallicFactor;
	else
		roughness = texture(textures[material.metallicRoughnessTextureIndex], uv).g;

	float metallic;
	if(material.metallicRoughnessTextureIndex == INVALID_TEXTURE)
		metallic = material.metallicFactor;
	else
		metallic = texture(textures[material.metallicRoughnessTextureIndex], uv).b;

	// Add striped pattern to roughness based on vertex position
#ifdef ROUGHNESS_PATTERN
	roughness = max(roughness, step(fract(inWorldPos.y * 2.02), 0.5));
#endif

	// Specular contribution
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < env.numOmniLights; i++) {
		vec3 L = normalize(env.omniLights[i].position.xyz - position);
		Lo += BRDF(L, V, N,albedo,env.omniLights[i].color.rgb, metallic, roughness);
	};

	// Combine with ambient
	vec3 color = albedo * 0.02;
	color += Lo;

	// apply occlution if exists
	if(material.occlusionTextureIndex != INVALID_TEXTURE)
	{
		color *= texture(textures[material.occlusionTextureIndex],uv).r;
	}

	// Tone mapping and gamma correct
	color = Uncharted2Tonemap(color * EXPOSURE);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	color = pow(color, vec3(0.4545));

	outColor = vec4(color, 1.0);
}
