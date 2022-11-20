#version 450

//inputs
layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 textCoords;

// outputs
layout (location = 0) out vec4 outColor;

// samplers
layout(set = 1, binding = 0) uniform sampler2D colSampler;
layout(set = 1, binding = 1) uniform sampler2D normSampler;
layout(set = 1, binding = 2) uniform sampler2D roughSampler;
layout(set = 1, binding = 3) uniform sampler2D aoSampler;


struct PointLight
{
    vec4 position;
    vec4 color;
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
mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv )
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );

    // solve the linear system 
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
   }

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord )
{
    // assume N, the interpolated vertex normal and V, the view vector (vertex to eye) 
    vec3 map = texture( normSampler, texcoord ).xyz;
    // WITH_NORMALMAP_UNSIGNED
    map = map * 255./127. - 128./127.;
    // WITH_NORMALMAP_2CHANNEL
    // map.z = sqrt( 1. - dot( map.xy, map.xy ) );
    // WITH_NORMALMAP_GREEN_UP
    // map.y = -map.y;
    mat3 TBN = cotangent_frame( N, -V, texcoord );
    return normalize( TBN * map );
}


void main()
{
    // pre-calculations shared with all lights
    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);
    //vec3 surfaceNormal = normalize(fragNormalWorld); // not used as normal map is used
    vec3 cameraPosWorld = ubo.inverseView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);
    vec3 surfaceNormal = perturb_normal(normalize(fragNormalWorld), viewDirection, textCoords.st);


    // directional light
    vec3 sunDirection = normalize(ubo.directionalLight.direction.xyz);
    float sunDiff = max(dot(surfaceNormal, sunDirection), 0.0);
    vec3 sunDiffuse = sunDiff * (ubo.directionalLight.color.xyz * ubo.directionalLight.color.w);  


    // point lights
    for(int i = 0; i < ubo.numLights; i++)
    {
        // per-light calculations
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);

        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;

        diffuseLight += intensity * cosAngIncidence;

        //specular component of light
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        blinnTerm = clamp(blinnTerm, 0, 1);
        blinnTerm = pow(blinnTerm, 512.0);  // higher power -> sharper light
        specularLight += intensity * blinnTerm;
    }
 
    // gl_FrontFacing can be used to apply lighting only on front facing surface
    vec4 sampledColor = texture(colSampler, textCoords) * texture(aoSampler, textCoords).r;

	outColor = vec4((
            (diffuseLight * sampledColor.rgb) + 
            (specularLight * texture(roughSampler, textCoords).r * sampledColor.rgb) +
            (sunDiffuse * sampledColor.rgb) 
     ), 1.0);    
}