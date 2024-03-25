#version 450

// inputs
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;
layout (location = 4) in vec4 tangent;

// outputs
layout (location = 0) out vec3 _normal;
layout (location = 1) out vec2 _uv;
layout (location = 2) out vec3 _position;
layout (location = 3) out vec4 _tangent;

layout (set = 1, binding = 0) uniform SceneUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
} scene;

layout (set = 2, binding = 0) uniform TransformUbo
{
    mat4 model;
    mat4 normal;
} transform;


void main()
{
	gl_Position = scene.projection * scene.view * transform.model * vec4(position, 1.0);
	_uv = uv;

    // vertex pos in viewspace
	_position = vec3(scene.view * transform.model * vec4(position, 1.0));

    // normal in view space
    mat3 normalMatrix = transpose(inverse(mat3(scene.view * transform.model)));
	_normal = normalMatrix * normalize(normal);	
	_tangent = mat4(normalMatrix) * normalize(tangent);
}