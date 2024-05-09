#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#define INVALID_TEXTURE 4294967295
// inputs
layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 position;
layout (location = 3) in vec4 tangent;

// outputs
layout (location = 0) out vec4 _position;
layout (location = 1) out vec4 _albedo;
layout (location = 2) out vec4 _normal;
layout (location = 3) out vec4 _material;

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


layout (constant_id = 0) const bool ALPHA_MASK = true;
layout (constant_id = 1) const float ALPHA_MASK_CUTOFF = 1.0f;
layout (constant_id = 2) const float nearPlane = 0.1;
layout (constant_id = 3) const float farPlane = 64.0;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));	
}

void main()
{
    _position = vec4(position, linearDepth(gl_FragCoord.z));

	if(material.baseColorTextureIndex == INVALID_TEXTURE)
		_albedo = material.baseColorFactor;
	else 
		_albedo = vec4(pow(texture(textures[material.baseColorTextureIndex], uv).rgb, vec3(1.0)),1);

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

    _material = vec4(roughness, metallic, ao, 1.0);

	if (material.alphaCutoff < 0.5) {
		discard;
	}


    vec3 N = normal;
	if(material.normalTextureIndex != INVALID_TEXTURE)
	{
		vec3 N = normalize(normal);
		vec3 T = normalize(tangent).xyz;
		vec3 B = cross(normal, tangent.xyz) * tangent.w;
		mat3 TBN = mat3(T, B, N);
		_normal = vec4(TBN * normalize(texture(textures[material.normalTextureIndex], uv).xyz * 2.0 - 1.0),1.0);
	}
	else
	{
		_normal = vec4(N, 1.0);
	}
	
}