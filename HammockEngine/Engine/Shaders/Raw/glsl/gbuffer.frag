#version 450
#extension GL_EXT_nonuniform_qualifier : enable
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

layout(set = 2, binding = 0) uniform sampler2D textures[];

layout (set = 0, binding = 0) uniform GlobalUbo
{
    mat4 projection;
    mat4 view;
} ubo;

// push constants
layout (push_constant) uniform Push
{
    mat4 modelMatrix; // model matrix
    mat4 normalMatrix; // using mat4 bcs alignment requirements
} push;


layout (constant_id = 0) const bool ALPHA_MASK = true;
layout (constant_id = 1) const float ALPHA_MASK_CUTOFF = 1.0f;
layout (constant_id = 2) const float nearPlane = 0.1;
layout (constant_id = 3) const float farPlane = 64.0;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));	
}

void main()
{
    _position = vec4(position, linearDepth(gl_FragCoord.z));
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
	_normal = vec4(normalize(tnorm), 1.0);
}