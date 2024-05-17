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

layout(set = 0, binding = 1) uniform sampler2D textures[];

layout(set = 3, binding = 0) uniform sampler2D positionSampler;

layout(set = 0, binding = 2) uniform sampler2D prefilteredSampler;
layout(set = 0, binding = 3) uniform sampler2D brdfLUTSampler;
layout(set = 0, binding = 4) uniform sampler2D irradinaceSampler;

layout (set = 0, binding = 0) uniform SceneUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
	
	float exposure;
	float gamma;
	float whitePoint;

	OmniLight omniLights[1000];
    uint numOmniLights;
} scene;

layout (set = 2, binding = 0) uniform MaterialPropertyUbo
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

layout (constant_id = 0) const bool ALPHA_MASK = true;
layout (constant_id = 1) const float ALPHA_MASK_CUTOFF = 1.0f;
layout (constant_id = 2) const float nearPlane = 0.1;
layout (constant_id = 3) const float farPlane = 64.0;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));	
}

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
vec3 F_Schlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 specularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 albedo, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);

	// Light color fixed
	vec3 lightColor = vec3(1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0) {
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness); 
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = F_Schlick(dotNV, F0);		
		vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);		
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);			
		color += (kD * albedo / PI + spec) * dotNL;
	}

	return color;
}

// Function to calculate spherical UV coordinates from a direction vector
vec2 toSphericalUV(vec3 direction)
{
    float phi = atan(direction.z, direction.x);
    float theta = acos(direction.y);
    float u = phi / (2.0 * PI) + 0.5;
    float v = theta / PI;
    return vec2(u, v);
}

// From http://filmicgames.com/archives/75
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


vec3 prefilteredReflection(vec2 R, float roughness)
{
	const float MAX_REFLECTION_LOD = 9.0; // todo: param/const
	float lod = roughness * MAX_REFLECTION_LOD;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	vec3 a = textureLod(prefilteredSampler, R, lodf).rgb;
	vec3 b = textureLod(prefilteredSampler, R, lodc).rgb;
	return mix(a, b, lod - lodf);
}

void main()
{
    vec4 albedo;
    if(material.baseColorTextureIndex == INVALID_TEXTURE){
        albedo = material.baseColorFactor;
    }
	else {
        albedo = texture(textures[material.baseColorTextureIndex], uv);
        vec3 albedo_corrected = albedo.xyz;
        albedo = vec4(pow(albedo_corrected, vec3(scene.gamma)), albedo.a);
    }

    float alpha = albedo.a;

    float roughness;
	if(material.metallicRoughnessTextureIndex == INVALID_TEXTURE)
		roughness = material.roughnessFactor;
	else
		roughness = texture(textures[material.metallicRoughnessTextureIndex], uv).g;

	float metallic;
	if(material.metallicRoughnessTextureIndex == INVALID_TEXTURE)
		metallic = material.metallicFactor;
	else
		metallic = texture(textures[material.metallicRoughnessTextureIndex], uv).b;

	float ao;
	if(material.occlusionTextureIndex == INVALID_TEXTURE)
		ao = 1.0;
	else 
		ao = texture(textures[material.occlusionTextureIndex], uv).r;

    vec3 N = normalize(normal);
    vec3 V = normalize(scene.inverseView[3].xyz - position);
    vec3 R = reflect(V, N);
		

    // sample depth from
    vec4 sampled_position = texture(positionSampler, gl_FragCoord.xy / vec2(1920.0, 1080.0));
    float depth = sampled_position.a;

    if ((depth != 0.0) && (linearDepth(gl_FragCoord.z) > depth))
	{
		discard;
	};


    vec3 F0 = vec3(0.04); 
	F0 = mix(F0, albedo.xyz, metallic);

    vec3 Lo = vec3(0.0);
	 for(int i = 0; i < scene.numOmniLights; ++i) {
		vec3 L = normalize(scene.omniLights[i].position.xyz - position);
		Lo += specularContribution(L, V, N, F0, albedo.xyz, metallic, roughness);
	}   


    vec2 brdf = texture(brdfLUTSampler, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 reflection = prefilteredReflection(toSphericalUV(R), roughness).rgb;	
	vec3 irradiance = texture(irradinaceSampler, toSphericalUV(N)).rgb;

	// Diffuse based on irradiance
	vec3 diffuse = irradiance * albedo.xyz;	

	vec3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);

	// Specular reflectance
	vec3 specular = reflection * (F * brdf.x + brdf.y);

	// Ambient part
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;	  
	vec3 ambient = (kD * diffuse + specular + Lo) * vec3(ao);
	
	vec3 color = ambient * ao;

    // Tone mapping
	color = Uncharted2Tonemap(color * scene.exposure);
	color = color * (1.0f / Uncharted2Tonemap(vec3(scene.whitePoint)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / scene.gamma));

    outColor = vec4(ambient, alpha);
}