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


layout (set = 0, binding = 0) uniform ShaderData
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
} data;

// push constants
layout (push_constant) uniform Push
{
    mat4 modelMatrix; // model matrix
    mat4 normalMatrix; // using mat4 bcs alignment requirements
    //int albedo_index;
} push;


void main()
{
    gl_Position = data.projection * data.view * push.modelMatrix * vec4(position, 1.0);

    _uv = uv;

    // vertex pos in viewspace
	_position = vec3(data.view * push.modelMatrix * vec4(position, 1.0));


	_normal = mat3(push.normalMatrix) * normalize(normal);	
	_tangent = mat4(push.normalMatrix) * normalize(tangent);
}