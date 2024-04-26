#version 450

// inputs
layout (location = 0) in vec2 inUv;

// outputs
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform SceneUbo
{
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 sunPosition;
    vec4 sunColor;
    vec4 baseSkyColor;
    vec4 gradientSkyColor;
} data;

layout(set = 0, binding = 1) uniform sampler2D noiseSampler;

layout (push_constant) uniform PushConstants {
    float resX;
    float resY;
    float elapsedTime;
    float maxSteps;
    float marchSize;
} push;


float noise(vec3 x ) {
  vec3 p = floor(x);
  vec3 f = fract(x);
  f = f*f*(3.0-2.0*f);

  vec2 uv = (p.xy+vec2(37.0,239.0)*p.z) + f.xy;
  vec2 tex = textureLod(noiseSampler,(uv+0.5)/256.0,0.0).yx;

  return mix( tex.x, tex.y, f.z ) * 2.0 - 1.0;
}

float fbm(vec3 p) {
  vec3 q = p + push.elapsedTime * 0.5 * vec3(1.0, -0.2, -1.0);
  float g = noise(q);

  float f = 0.0;
  float scale = 0.5;
  float factor = 2.02;

  for (int i = 0; i < 6; i++) {
      f += scale * noise(q);
      q *= factor;
      factor += 0.21;
      scale *= 0.5;
  }

  return f;
}

float sdSphere(vec3 p, float radius) {
    return length(p) - radius;
}

#define RADIUS 2.0

float scene(vec3 p) {
 float distance = sdSphere(p, RADIUS);

  float f = fbm(p);

  return -distance + f;
}


vec4 raymarch(vec3 rayOrigin, vec3 rayDirection) {
  float depth = 0.0;
  vec3 p = rayOrigin + depth * rayDirection;
  vec3 sunDirection = normalize(data.sunPosition.rgb);
  
  vec4 res = vec4(0.0);

  for (int i = 0; i < push.maxSteps; i++) {
    float transmittance = scene(p);

    // We only draw the transmittance if it's greater than 0
    if (transmittance > 0.0) {
      // Directional derivative
      // For fast diffuse lighting
      float diffuse = clamp((scene(p) - scene(p + 0.3 * sunDirection)) / 0.3, 0.0, 1.0 );
      vec3 lin = vec3(0.60,0.60,0.75) * 1.1 + 0.8 * vec3(1.0,0.6,0.3) * diffuse;
      vec4 color = vec4(mix(vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, 0.0), transmittance), transmittance );
      color.rgb *= lin;
      color.rgb *= color.a;
      res += color * (1.0 - res.a);
    }

    depth += push.marchSize;
    p = rayOrigin + depth * rayDirection;
  }

  return res;
}

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(push.resX, push.resY);
    uv -= 0.5;
    uv.x *= push.resX / push.resY;

    // Flip the Y-coordinate for Vulkan
    uv.y *= -1.0;

    // Ray Origin - camera
    vec3 ro = data.inverseView[3].xyz; // scene.inverseView[3].xyz is camera position

    // Ray Direction
    vec3 rd = normalize(vec3(uv, 1.0));

    // Transform ray direction by the camera's orientation
    rd = (data.inverseView * vec4(rd, 0.0)).xyz;

  
    vec3 color = vec3(0.0);
    // Sun and Sky
    vec3 sunDirection = normalize(data.sunPosition.xyz);
    sunDirection.y *= -1;
    float sun = clamp(dot(sunDirection, rd), 0.0, 1.0 );
    // Base sky color
    color = data.baseSkyColor.rgb;
    // Add vertical gradient
    color -= data.gradientSkyColor.a * data.gradientSkyColor.rgb * rd.y;
    // Add sun color to sky
    color += 0.5 * data.sunColor.rgb * pow(sun, 10.0);
    // cloud
    vec4 res = raymarch(ro, rd);
    color = color * (1.0 - res.a) + res.rgb;

    outColor = vec4(color, 1.0);
}