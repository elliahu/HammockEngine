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


void main()
{
    vec3 N = normalize(normal);
	if(material.normalTextureIndex != INVALID_TEXTURE)
	{
		vec3 N = normalize(normal);
		vec3 T = normalize(tangent).xyz;
		vec3 B = cross(normal, tangent.xyz) * tangent.w;
		mat3 TBN = mat3(T, B, N);
		N = TBN * normalize(texture(textures[material.normalTextureIndex], uv).rgb * 2.0 - vec3(1.0));
	}

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
		float diff = max(dot(N,L),0.0);
        vec3 diffuse = diff * env.omniLights[i].color.xyz;

        vec3 R = reflect(-L, N);  
        float spec = pow(max(dot(V, R), 0.0), roughness);
        vec3 specular = spec * env.omniLights[i].color.xyz;  

        Lo += (diffuse + specular + (albedo * 0.1)) * albedo;
	};

	outColor = vec4(Lo, 1.0);
}
