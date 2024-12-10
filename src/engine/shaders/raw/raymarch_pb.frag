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
layout(set = 0, binding = 2) uniform sampler2D blueNoiseSampler;

layout (push_constant) uniform PushConstants {
    float resX;
    float resY;
    float elapsedTime;
    float maxSteps;
	float maxStepsLight;
	float marchSize;
	float absorbtionCoef;
	float scatteringAniso;
} push;

#define RADIUS 2.0
#define PI 3.14159265359

float noise(vec3 x ) {
  vec3 p = floor(x);
  vec3 f = fract(x);
  f = f*f*(3.0-2.0*f);

  vec2 uv = (p.xy+vec2(37.0,239.0)*p.z) + f.xy;
  vec2 tex = textureLod(noiseSampler,(uv+0.5)/256.0,0.0).yx;

  return mix( tex.x, tex.y, f.z ) * 2.0 - 1.0;
}

float fbm(vec3 p, bool lowRes) {
  vec3 q = p + push.elapsedTime * 0.5 * vec3(1.0, -0.2, -1.0);
  float g = noise(q);

  float f = 0.0;
  float scale = 0.5;
  float factor = 2.02;
 
  int maxOctave = 6;

  if(lowRes) {
    maxOctave = 3;
  }

  for (int i = 0; i < maxOctave; i++) {
      f += scale * noise(q);
      q *= factor;
      factor += 0.21;
      scale *= 0.5;
  }

  return f;
}

float SDF(vec3 p, float radius) {
    return length(p) - radius;
}

float density(vec3 p, bool lowRes) {
  float distance = SDF(p, 1.2);

  float f = fbm(p, lowRes);

  return -distance + f;
}

float BeersLaw (float dist, float absorption) {
  return exp(-dist * absorption);
}

float lightmarch(vec3 position, vec3 rayDirection) {
  vec3 lightDirection = normalize(data.sunPosition.rgb);
  float totalDensity = 0.0;
  float marchSize = 0.03;

  for (int step = 0; step < push.maxStepsLight; step++) {
      position += lightDirection * marchSize * float(step);

      float lightSample = density(position, true);
      totalDensity += lightSample;
  }

  float transmittance = BeersLaw(totalDensity, push.absorbtionCoef);
  return transmittance;
}

float HenyeyGreenstein(float g, float mu) {
  float gg = g * g;
	return (1.0 / (4.0 * PI))  * ((1.0 - gg) / pow(1.0 + gg - 2.0 * g * mu, 1.5));
}

float raymarch(vec3 rayOrigin, vec3 rayDirection, float offset) {
  float depth = 0.0;
  depth += push.marchSize * offset;
  vec3 p = rayOrigin + depth * rayDirection;
  vec3 sunDirection = normalize(data.sunPosition.rgb);
  
  float totalTransmittance = 1.0;
  float lightEnergy = 0.0;

  float phase = HenyeyGreenstein(push.scatteringAniso, dot(rayDirection, sunDirection));

  for (int i = 0; i < push.maxSteps; i++) {
    float density = density(p, false);

    // We only draw the transmittance if it's greater than 0
    if (density > 0.0) {
      float lightTransmittance = lightmarch(p, rayDirection);
      float luminance = 0.025 + density * phase;

      totalTransmittance *= lightTransmittance;
      lightEnergy += totalTransmittance * luminance;
    }

    depth += push.marchSize;
    p = rayOrigin + depth * rayDirection;
  }

  return lightEnergy;
}

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(push.resX, push.resY);
    uv -= 0.5;
    uv.x *= push.resX / push.resY;

    // Ray Origin - camera
    vec3 ro = data.inverseView[3].xyz; // scene.inverseView[3].xyz is camera position

    // Ray Direction
    vec3 rd = normalize(vec3(uv, 1.0));

    // Transform ray direction by the camera's orientation
    rd = (data.view * vec4(rd, 0.0)).xyz;


  
  vec3 color = vec3(0.0);
  
  // Sun and Sky
  vec3 sunColor = data.sunColor.rgb;
  vec3 sunDirection = normalize(data.sunPosition.rgb);
  float sun = clamp(dot(sunDirection, rd), 0.0, 1.0);
  // Base sky color
  color = data.baseSkyColor.rgb;
  // Add vertical gradient
  color -= data.gradientSkyColor.a * data.gradientSkyColor.rgb * rd.y;
  // Add sun color to sky
  color += 0.5 * sunColor * pow(sun, 10.0);

  float blueNoise = texture(blueNoiseSampler, gl_FragCoord.xy / 1024.0).r;
  float offset = fract(blueNoise);

  // Cloud
  float res = raymarch(ro, rd, offset);
  color = color + sunColor * res;

  outColor = vec4(color, 1.0);
}