#ifndef TYPES_GLSL
#define TYPES_GLSL

struct OmniLight
{
    vec4 position;
    vec4 color;
    float linear;
    float quadratic;
    float _padding1;     // 4 bytes
    float _padding2;     // 4 bytes
};

#endif // CONSTS_GLSL