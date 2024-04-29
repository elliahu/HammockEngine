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

// Constants
const float PI = 3.14159265359;

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

// Cook-Torrance BRDF
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}



vec3 CookTorranceBRDF(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness, float metallic)
{
    vec3 H = normalize(V + L);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;

    return nominator / denominator;
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
        float diff = max(dot(N, L), 0.0);
        vec3 specular = CookTorranceBRDF(N, V, L, albedo, roughness, metallic);
        vec3 diffuse = diff * env.omniLights[i].color.xyz;

        Lo += (diffuse + specular + ambient) * albedo;
    };

    if(!isSkybox(material))
        outColor = vec4(Lo, 1.0);
    else 
        outColor = vec4(albedo, 1.0);
}
