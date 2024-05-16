#version 450

layout(set = 0, binding = 2) uniform sampler2D environmentSampler;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outFragColor;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outMaterial;

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
// From http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 Uncharted2Tonemap(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

const float EXPOSURE = 4.0;
const float GAMMA = 2.2;

void main() 
{
    // Transform UV coordinates to a direction in view space
    vec4 direction_view = inverse(scene.projection * scene.view) * vec4(in_uv * 2.0 - 1.0, 1.0, 1.0);
    vec3 direction = normalize(direction_view.xyz / direction_view.w);
    
    // Convert 3D direction to 2D UV coordinates using equirectangular projection
    vec2 uv;
    uv.x = atan(direction.z, direction.x) / (2.0 * 3.14159265358979323846) + 0.5;
    uv.y = asin(direction.y) / 3.14159265358979323846 + 0.5;
    uv.y *= -1;
    // Sample the environment map in the calculated direction
    vec3 color = texture(environmentSampler, uv).rgb;
    // Tone mapping
	color = Uncharted2Tonemap(color * scene.exposure);
	color = color * (1.0f / Uncharted2Tonemap(vec3(scene.whitePoint)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / scene.gamma));

    // Output the environment color as fragment color
    outFragColor = vec4(color, 1.0);

    outPosition = vec4(0);
    outNormal = vec4(0);
    outMaterial = vec4(-1);
}
