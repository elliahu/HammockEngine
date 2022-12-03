#version 450

//inputs
layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 textCoords;
layout (location = 4) in mat3 TBN;

// outputs
layout (location = 0) out vec4 outColor;

// samplers
layout(set = 1, binding = 0) uniform sampler2D colSampler;
layout(set = 1, binding = 1) uniform sampler2D normSampler;
layout(set = 1, binding = 2) uniform sampler2D roughSampler;
layout(set = 1, binding = 3) uniform sampler2D metalSampler;
layout(set = 1, binding = 4) uniform sampler2D aoSampler;
layout(set = 1, binding = 5) uniform sampler2D disSampler;


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
    vec4 ambientLightColor; // w is intensity
    DirectionalLight directionalLight;
    PointLight pointLights[10];
    int numLights;
} ubo;

// push constants
layout (push_constant) uniform Push
{
    mat4 modelMatrix; // model matrix
    mat4 normalMatrix; // using mat4 bcs alignment requirements
} push;


// Normal mapping without precalculated tangents
// "Followup: Normal Mapping Without Precomputed Tangents" from http://www.thetenthplanet.de/archives/1180


// displacement https://learnopengl.com/Advanced-Lighting/Parallax-Mapping
float heightScale = 0.05;
float numLayers = 8.0;
vec2 displace(vec2 uv, vec3 viewDir)
{ 
    float layerDepth = 1.0 / numLayers;
	float currLayerDepth = 0.0;
	vec2 deltaUV = viewDir.xy * heightScale / (viewDir.z * numLayers);
	vec2 currUV = uv;
	float height = 1.0 - textureLod(disSampler, currUV, 0.0).a;
	for (int i = 0; i < numLayers; i++) {
		currLayerDepth += layerDepth;
		currUV -= deltaUV;
		height = 1.0 - textureLod(disSampler, currUV, 0.0).a;
		if (height < currLayerDepth) {
			break;
		}
	}
	vec2 prevUV = currUV + deltaUV;
	float nextDepth = height - currLayerDepth;
	float prevDepth = 1.0 - textureLod(disSampler, prevUV, 0.0).a - currLayerDepth + layerDepth;
	return mix(currUV, prevUV, nextDepth / (nextDepth - prevDepth));
}

void main()
{
    // pre-calculations shared with all lights
    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);
    vec3 cameraPosWorld = ubo.inverseView[3].xyz;
    vec3 viewDirection = normalize((TBN * cameraPosWorld) - (TBN * fragPosWorld));
    vec2 uv =  textCoords;//displace(textCoords, viewDirection);

    // calculating normal
    vec3 surfaceNormal = textureLod(normSampler, uv, 0.0).rgb;
    // Discard fragments at texture border
	if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
		discard;
	}
    surfaceNormal = normalize(surfaceNormal);


    // directional light
    vec3 sunDirection = normalize(TBN * ubo.directionalLight.direction.xyz);
    float sunDiff = max(dot(surfaceNormal, sunDirection), 0.0);
    vec3 sunDiffuse = sunDiff * (ubo.directionalLight.color.xyz * ubo.directionalLight.color.w); 
    
    //specular component of light
    vec3 halfAngle = normalize(sunDirection + viewDirection);
    float blinnTerm = dot(surfaceNormal, halfAngle);
    blinnTerm = clamp(blinnTerm, 0, 1);
    blinnTerm = pow(blinnTerm, 512.0);  // higher power -> sharper light
    specularLight += (ubo.directionalLight.color.xyz * ubo.directionalLight.color.w) * blinnTerm;


    // point lights
    for(int i = 0; i < ubo.numLights; i++)
    {
        // per-light calculations
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = (TBN * light.position.xyz) -  (TBN * fragPosWorld);
        float attenuation = 1.0 / (light.terms.x * dot(directionToLight, directionToLight) + light.terms.y * length(directionToLight) + light.terms.z );
        directionToLight = normalize(directionToLight);

        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;

        diffuseLight += intensity * cosAngIncidence;

        //specular component of light
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 1024.0 * (1.0 + texture(roughSampler, uv).r));  // higher power -> sharper light
        specularLight += intensity * blinnTerm;
    }
 
    // gl_FrontFacing can be used to apply lighting only on front facing surface
    vec4 sampledColor = texture(colSampler, uv) * texture(aoSampler, uv).r;

	outColor = vec4((
            (diffuseLight * sampledColor.rgb) + 
            (specularLight * ( texture(metalSampler, uv).r)  * sampledColor.rgb) +
            (sunDiffuse * sampledColor.rgb)
     ), 1.0);    
}