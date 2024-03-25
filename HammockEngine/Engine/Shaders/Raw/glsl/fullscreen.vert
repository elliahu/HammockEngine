#version 450

// inputs ignored but bound
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 uv;
layout (location = 4) in vec4 tangent;

//outputs
layout (location = 0) out vec2 _uv;

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

out gl_PerVertex
{
	vec4 gl_Position;
};
 
void main() 
{
    _uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);	
	gl_Position = vec4(_uv * 2.0f - 1.0f, 0.0f, 1.0f);
}