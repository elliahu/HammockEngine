#version 450
#extension GL_EXT_nonuniform_qualifier : enable

#define INVALID_TEXTURE 4294967295


// inputs
layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec4 inTangent;

// outputs
layout (location = 0) out vec4 outColor;


struct OmniLight
{
    vec4 position;
    vec4 color;
};

layout(set = 0, binding = 0) uniform Environment
{
    OmniLight omniLights[10];
    uint numOmniLights;
} env;

layout(set = 0, binding = 1) uniform sampler2D textures[];

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

layout (set = 3, binding = 0) uniform MaterialPropertyUbo
{
    vec4 baseColorFactor;
    uint baseColorTextureIndex;
    uint normalTextureIndex;
    uint metallicRoughnessTextureIndex;
    uint occlusionTextureIndex;
    float alphaCutoff;
	float metallicFactor;
	float roughnessFactor;
} material;

layout(set = 4, binding = 0) uniform sampler2D noiseSampler;

layout (push_constant) uniform PushConstants {
    vec2 resolution;
} push;

#define MAX_STEPS 100

float sdSphere(vec3 p, float radius) {
    return length(p) - radius;
}

float _scene(vec3 p) {
  float distance = sdSphere(p, 1.0);
  return -distance;
}

const float MARCH_SIZE = 0.08;

vec4 raymarch(vec3 rayOrigin, vec3 rayDirection) {
  float depth = 0.0;
  vec3 p = rayOrigin + depth * rayDirection;
  
  vec4 res = vec4(0.0);

  for (int i = 0; i < MAX_STEPS; i++) {
    float density = _scene(p);

    // We only draw the density if it's greater than 0
    if (density > 0.0) {
      vec4 color = vec4(mix(vec3(1.0,1.0,1.0), vec3(0.0, 0.0, 0.0), density), density );
      color.rgb *= color.a;
      res += color*(1.0-res.a);
    }

    depth += MARCH_SIZE;
    p = rayOrigin + depth * rayDirection;
  }

  return res;
}

void main() {
   vec2 uv = gl_FragCoord.xy / push.resolution.xy;
  uv -= 0.5;
  uv.x *= push.resolution.x / push.resolution.y;

  // Ray Origin - camera
  vec3 ro = scene.inverseView[3].xyz; // scene.inverseView[3].xyz is camera position
  // Ray Direction
  vec3 rd = normalize((scene.inverseView * vec4(vec3(uv, 1.0), 0.0)).xyz);

  
  vec3 color = vec3(0.0);
  vec4 res = raymarch(ro, rd);
  color = res.rgb;

  outColor = vec4(color, 1.0);
}