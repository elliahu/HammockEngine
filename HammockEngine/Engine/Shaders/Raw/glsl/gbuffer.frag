#version 450
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


layout(set = 1, binding = 0) uniform sampler2D albedoSampler;
layout(set = 1, binding = 1) uniform sampler2D normSampler;
layout(set = 1, binding = 2) uniform sampler2D roughMetalSampler;
layout(set = 1, binding = 3) uniform sampler2D occlusionSampler;

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

// push constants
layout (push_constant) uniform Push
{
    mat4 modelMatrix; // model matrix
    mat4 normalMatrix; // using mat4 bcs alignment requirements
} push;


layout (constant_id = 0) const bool ALPHA_MASK = false;
layout (constant_id = 1) const float ALPHA_MASK_CUTOFF = 0.0f;

void main()
{
    _position = vec4(position, 1.0);
    _albedo = texture(albedoSampler, uv);
    _material = vec4(texture(roughMetalSampler, uv).g, texture(roughMetalSampler, uv).b, texture(occlusionSampler, uv).r, 1.0);

    if (ALPHA_MASK) {
		if (_albedo.a < ALPHA_MASK_CUTOFF) {
			discard;
		}
	}

    // Calculate normal in tangent space
	vec3 N = normalize(normal);
	vec3 T = normalize(tangent).xyz;
	vec3 B = cross(normal, tangent.xyz) * tangent.w;
	mat3 TBN = mat3(T, B, N);
    vec3 tnorm = TBN * normalize(texture(normSampler, uv).xyz * 2.0 - vec3(1.0));
	_normal = vec4(tnorm, 1.0);
}