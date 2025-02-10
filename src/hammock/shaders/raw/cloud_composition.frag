#version 450

layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D sceneColorTex;
layout (set = 0, binding = 1) uniform sampler2D sceneDepthTex;
layout (set = 0, binding = 2) uniform sampler2D cloudColorTex;
layout (set = 0, binding = 3) uniform sampler2D cloudViewSpaceDepthTex;

vec4 mixAlpha(vec4 colorA, vec4 colorB) {
    float alphaResult = colorA.a + colorB.a * (1.0 - colorA.a);
    vec3 colorResult = (colorA.rgb * colorA.a + colorB.rgb * colorB.a * (1.0 - colorA.a)) / max(alphaResult, 1e-5);
    return vec4(colorResult, alphaResult);
}

float linearizeDepth(float depth, float near, float far) {
    return (2.0 * near * far) / (far + near - depth * (far - near));
}

void main() {
    //    float sceneDepth = linearizeDepth(texture(sceneDepthTex, uv).r, 0.1, 64.0);
    //    float cloudDepth = linearizeDepth(texture(cloudViewSpaceDepthTex, uv).r, 0.1, 64.0);
    //    vec4 sceneColor = texture(sceneColorTex, uv);
    //
    //    // Cloud is visible
    //    if(sceneDepth > cloudDepth){
    //        vec4 cloudColor = texture(cloudColorTex, uv);
    //        outColor = mixAlpha(cloudColor, sceneColor);
    //    }
    //    else{
    //        outColor = sceneColor;
    //    }

    vec4 sceneColor = texture(sceneColorTex, uv);
    vec4 cloudColor = texture(cloudColorTex, uv);
    outColor = mixAlpha(cloudColor, sceneColor);

}
