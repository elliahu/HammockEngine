#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable

//inputs
layout (location = 0) in vec2 uv;
 
// outputs
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D environmentSampler;

// gbuffer attachments  
layout(set = 3, binding = 0) uniform sampler2D positionSampler;
layout(set = 3, binding = 1) uniform sampler2D albedoSampler;
layout(set = 3, binding = 2) uniform sampler2D normalSampler;
layout(set = 3, binding = 3) uniform sampler2D materialPropertySampler;

struct OmniLight
{
    vec4 position;
    vec4 color;
};

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

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

#define EPSILON 0.15
#define SHADOW_OPACITY 0.0

void main()
{
    vec3 N = normalize(texture(normalSampler, uv).rgb);
    vec3 position = texture(positionSampler, uv).rgb;
    vec3 V = normalize(-position);
    vec3 albedo = texture(albedoSampler, uv).rgb;
    vec3 material = texture(materialPropertySampler, uv).rgb;
    float roughness = material.r;
    float metallic = material.g;
    float ao = material.b;

	if(material == vec3(-1.0)) // background pixels are skipped
    {
        outColor = vec4(albedo, 1.0);
        return;
    }

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0);
    for(int i = 0; i < scene.numOmniLights; ++i) {
        // calculate per-light radiance
        vec3 L = normalize(scene.omniLights[i].position.xyz - position);
        
        /* // Shadow
        // TODO sample from omnidirectional shadow map (shadowCubeMap) and decide if the fragment is in shadow
        vec3 shadowDir = normalize((scene.inverseView * vec4(L, 0.0)).xyz);
        float shadowDepth = texture(shadowCubeMap, -shadowDir).r;
        float lightDistance = length(scene.omniLights[i].position.xyz - position);
        float shadow = lightDistance <= shadowDepth + EPSILON ? 1.0 : SHADOW_OPACITY;
        */
        vec3 H = normalize(V + L);
        float distance    = length(scene.omniLights[i].position.xyz - position);
        float attenuation = 1.0 / (distance * distance);
        //attenuation = 1.0;
        vec3 radiance     = scene.omniLights[i].color.rgb * attenuation;

        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);    
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);     

        vec3 nominator    = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // prevent division by zero
        vec3 specular = nominator / denominator;

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += /*shadow * */ (specular + albedo / PI) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    // apply tone mapping
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2)); 

    outColor = vec4(color, 1.0);
}
