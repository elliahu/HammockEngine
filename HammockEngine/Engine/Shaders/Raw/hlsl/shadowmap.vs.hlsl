struct VSInput
{
	[[vk::location(0)]] float3 Position : POSITION0;
	[[vk::location(1)]] float3 Color : COLOR0;
	[[vk::location(2)]] float3 Normal : NORMAL0;
	[[vk::location(3)]] float3 Uv : TEXCOORD0;
	[[vk::location(4)]] float3 Tangent : TANGENT0;
};

struct PointLight
{
	float4 Position;
	float4 Color;
	float4 Terms;
};

struct DirectionalLight
{
	float4 Direction;
	float4 Color;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 inverseView;
	float4x4 depthBiasMVP;
	float4 ambientLightColor; // w is intensity
	DirectionalLight directionalLight;
	PointLight pointLights[10];
	int numLights;
};

cbuffer ubo : register(b0, space0) { UBO ubo; }

struct Push
{
	float4x4 modelMatrix;
	float4x4 normalMatrix;
};

[[vk::push_constant]] Push push;

struct VSOutput
{
	float4 Position : SV_POSITION;
};

VSOutput main(VSInput input, uint VertexIndex : SV_VertexID)
{
	VSOutput output = (VSOutput)0;

	output.Position = mul(ubo.depthBiasMVP, mul(push.modelMatrix, float4(input.Position, 1.0)));
	return output;
}
