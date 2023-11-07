#version 450
#extension GL_EXT_nonuniform_qualifier : enable

// inputs
layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 position;
layout (location = 3) in vec4 tangent;

// outputs
layout (location = 0) out vec4 outColor;


layout (set = 0, binding = 0) uniform ShaderData
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
} data;

layout(set = 0, binding = 1) uniform sampler2D textures[];



// push constants
layout (push_constant) uniform Push
{
    mat4 modelMatrix; // model matrix
    mat4 normalMatrix; // using mat4 bcs alignment requirements
    //int albedo_index;
} push;

float beers_law(float distance, float absorbtion)
{
    return exp(-distance * absorbtion);
}



void main()
{
    vec3 sphere_position = vec3(2,0,2);


    outColor = texture(textures[0], uv) ;//+ vec4(1) * (1 - beers_law(distance(vec3(0), position),0.04));
}
