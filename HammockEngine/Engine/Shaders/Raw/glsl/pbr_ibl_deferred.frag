#version 450

//inputs
layout (location = 0) in vec2 uv;
 
// outputs
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D prefilteredSampler;
layout(set = 0, binding = 3) uniform sampler2D brdfLUTSampler;
layout(set = 0, binding = 4) uniform sampler2D irradinaceSampler;

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
vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
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

const float EXPOSURE = 4.0;
const float GAMMA = 2.2;

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
    vec3 N = normalize(texture(normalSampler, uv).rgb);
    vec3 position = texture(positionSampler, uv).rgb;
    vec3 V = normalize(-position);
    vec3 R = reflect(-V, N); 
    // TODO transform the ray R into the view space
    R = mat3(scene.inverseView) * R;
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
    // Add contribution from other point lights
    for(int i = 0; i < env.numOmniLights; ++i) {
        vec3 L = normalize(env.omniLights[i].position.xyz - position);
        vec3 H = normalize(V + L);
        float distance    = length(env.omniLights[i].position.xyz - position);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = env.omniLights[i].color.rgb * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);    
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);     

        vec3 nominator    = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // prevent division by zero
        vec3 specular = nominator / denominator;

        // Add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (specular + albedo / PI) * radiance * NdotL;
    }


    vec2 brdf = texture(brdfLUTSampler, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 reflection = prefilteredReflection(toSphericalUV(R), roughness).rgb;	
	vec3 irradiance = texture(irradinaceSampler, toSphericalUV(N)).rgb;

	// Diffuse based on irradiance
	vec3 diffuse = irradiance * albedo;	

	vec3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);

	// Specular reflectance
	vec3 specular = reflection * (F * brdf.x + brdf.y);

	// Ambient part
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;	  
	vec3 ambient = (kD * diffuse + specular + Lo) * vec3(ao);
	
	vec3 color = ambient;

    // Tone mapping
	color = Uncharted2Tonemap(color * EXPOSURE);
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / GAMMA));

    outColor = vec4(ambient, 1.0);
}