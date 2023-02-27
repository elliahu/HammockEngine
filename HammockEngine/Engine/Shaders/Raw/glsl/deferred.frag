#version 450

//inputs
layout (location = 0) in vec2 uv;

// outputs
layout (location = 0) out vec4 outColor;

// gbuffer attachments  
layout(set = 1, binding = 0) uniform sampler2D positionSampler;
layout(set = 1, binding = 1) uniform sampler2D albedoSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D materialPropertySampler;

// shadowmap sampler
layout(set = 2, binding = 0) uniform sampler2D directionalLightDepthMap;

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


const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
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
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( directionalLightDepthMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0.0;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(directionalLightDepthMap, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}
	
	}
	return shadowFactor / count;
}

// compute shadows
int enablePCF = 1;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
 );


// MAIN
void main()
{
    
    vec3 viewPosition = ubo.inverseView[3].xyz;
    vec3 wolrdPosition = texture(positionSampler, uv).rgb;

    
    vec3 V = normalize(viewPosition - wolrdPosition);
    vec3 N = normalize(texture(normalSampler, uv).rgb);

    vec3 albedo =pow(texture(albedoSampler, uv).rgb, vec3(2.2));
    float roughness = texture(materialPropertySampler, uv).r;
    float metallic  = texture(materialPropertySampler, uv).g;
    float ao        = texture(materialPropertySampler, uv).b;

    // Discard fragments at texture border
	if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
		discard;
	}

    vec4 shadowCoord = ( biasMat * ubo.depthBiasMVP) * vec4(wolrdPosition, 1.0);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(albedo, albedo, metallic);
    
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < ubo.numLights; i++)
    {
        // per-light calculations
        PointLight light = ubo.pointLights[i];
        vec3 lightPosition = light.position.xyz;

        // calculate per-light radiance
        vec3 L = normalize(lightPosition - wolrdPosition);
        vec3 H = normalize(V + L);
        float distance = length(lightPosition - wolrdPosition);
        float attenuation = 1.0 / (light.terms.x * (distance * distance)) + (light.terms.y * distance) + (light.terms.z);
        vec3 radiance = light.color.xyz * light.color.w * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // directional light 
    vec3 L = normalize(-vec3(ubo.directionalLight.direction));
    vec3 H = normalize(V + L);
    vec3 radiance = ubo.directionalLight.color.xyz * ubo.directionalLight.color.w;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  

    float NdotL = max(dot(N, L), 0.0);   
    // add to outgoing radiance Lo
    float shadow = (enablePCF == 1) ? filterPCF(shadowCoord / shadowCoord.w) : textureProj(shadowCoord / shadowCoord.w, vec2(0.0));
    Lo += ((kD * albedo / PI + specular) * radiance * NdotL) * shadow;

 
    vec3 ambient = (ubo.ambientLightColor.xyz * ubo.ambientLightColor.w) * albedo * ao;
    
    vec3 color = ambient + Lo;
     
    // HDR tonemapping    
    color = color / (color + vec3(1.0)); 
    // gamma correction
    color = pow(color, vec3(1.0/2.2)); 

    outColor = vec4(color, 1.0);
}