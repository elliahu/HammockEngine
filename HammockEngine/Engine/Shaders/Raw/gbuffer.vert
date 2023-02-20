#version 450

// inputs
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;
layout (location = 4) in vec3 tangent;

// outputs
layout (location = 0) out vec3 _normal;
layout (location = 1) out vec2 _uv;
layout (location = 2) out vec3 _position;
layout (location = 3) out vec3 _tangent;

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

void main()
{
    
    //vec4 tmpPos = vec4(position.xyz, 1.0) + ubo.instancePos[gl_InstanceIndex];

	gl_Position = ubo.projection * ubo.view * push.modelMatrix * vec4(position, 1.0);
	
	_uv = uv;

	// Vertex position in world space
	_position = vec3(push.modelMatrix * vec4(position, 1.0));

	// Normal in world space
	_normal = mat3(push.normalMatrix) * normalize(normal);	
	_tangent = mat3(push.normalMatrix) * normalize(tangent);
}