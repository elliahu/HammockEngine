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
layout (location = 4) out vec3 _camera;


layout (set = 0, binding = 0) uniform SceneUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    mat4 depthBias;
    vec4 ambientColor;
} scene;

layout (set = 0, binding = 2) uniform TransformUbo
{
    mat4 model;
    mat4 normal;
} transform;

layout (set = 0, binding = 3) uniform MaterialPropertyUbo
{
    vec4 baseColorFactor;
    uint baseColorTextureIndex;
    uint normalTextureIndex;
    uint metallicRoughnessTextureIndex;
    uint occlusionTextureIndex;
    float alphaCutoff;
} material;


void main()
{
    gl_Position = scene.projection * scene.view * transform.model * vec4(position, 1.0);

    _uv = uv;

    // vertex pos in viewspace
	_position = vec3(scene.view * transform.model * vec4(position, 1.0));
	_normal = mat3(transform.normal) * normalize(normal);	
	_tangent = mat4(transform.normal) * normalize(tangent);
}