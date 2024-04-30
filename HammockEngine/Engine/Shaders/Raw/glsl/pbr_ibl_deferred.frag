#version 450

//inputs
layout (location = 0) in vec2 uv;
 
// outputs
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D environmentSampler;

// gbuffer attachments  
layout(set = 4, binding = 0) uniform sampler2D positionSampler;
layout(set = 4, binding = 1) uniform sampler2D albedoSampler;
layout(set = 4, binding = 2) uniform sampler2D normalSampler;
layout(set = 4, binding = 3) uniform sampler2D materialPropertySampler;

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

// Function to calculate spherical UV coordinates from a direction vector
vec2 toSphericalUV(vec3 direction)
{
    float phi = atan(direction.z, direction.x);
    float theta = acos(direction.y);
    float u = phi / (2.0 * PI) + 0.5;
    float v = theta / PI;
    return vec2(u, -v);
}

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
        // apply tone mapping
        vec3 color = albedo / (albedo + vec3(1.0));
        color = pow(color, vec3(1.0/2.2)); 
		outColor = vec4(color, 1.0);
		return;
	}

    vec3 Lo = vec3(0);

    // Dynamically compute reflection vector based on current view direction
    vec3 R = reflect(-V, N);

    // Sample the environment map using reflection vector
    vec2 sphericalUV = toSphericalUV(R);
    vec3 envMapColor = texture(environmentSampler, sphericalUV).rgb;

    // Apply Cook-Torrance BRDF
    float NDF = DistributionGGX(N, V, roughness);   
    float G   = GeometrySmith(N, V, V, roughness);    
    vec3 F    = fresnelSchlick(max(dot(N, V), 0.0), vec3(0.04));     

    vec3 nominator    = NDF * G * F;
    float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, V), 0.0) + 0.001; // prevent division by zero
    vec3 specular = nominator / denominator;

    // Add to outgoing radiance Lo
    Lo += (specular + albedo / PI) * envMapColor;

    // Add contribution from other point lights
    for(int i = 0; i < env.numOmniLights; ++i) {
        vec3 L = normalize(env.omniLights[i].position.xyz - position);
        vec3 H = normalize(V + L);
        float distance    = length(env.omniLights[i].position.xyz - position);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = env.omniLights[i].color.rgb * attenuation;

        // Cook-Torrance BRDF
        NDF = DistributionGGX(N, H, roughness);   
        G   = GeometrySmith(N, V, L, roughness);    
        F    = fresnelSchlick(max(dot(H, V), 0.0), vec3(0.04));     

        nominator    = NDF * G * F;
        denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // prevent division by zero
        specular = nominator / denominator;

        // Add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (specular + albedo / PI) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    // Apply tone mapping
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2)); 

    outColor = vec4(color, 1.0);
}
