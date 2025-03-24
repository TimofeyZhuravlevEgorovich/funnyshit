// lightingPS.hlsl

Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D materialTexture : register(t2);
SamplerState samplerState : register(s0);

cbuffer CameraBuffer : register(b0)
{
    float3 cameraPosition;
    float padding;
}

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 worldPosition : TEXCOORD1;
    float3 worldNormal : NORMAL;
};

float4 main(PixelInput input) : SV_Target
{
    float4 albedo = albedoTexture.Sample(samplerState, input.texCoord);
    float4 normalData = normalTexture.Sample(samplerState, input.texCoord);
    float4 material = materialTexture.Sample(samplerState, input.texCoord);

    float3 normal = normalize(normalData.xyz * 2.0f - 1.0f);
    float3 viewDir = normalize(cameraPosition - input.worldPosition);
    float3 lightDir = normalize(float3(0.0f, 1.0f, -1.0f));
    float diffuse = max(dot(normal, lightDir), 0.0f);

    return float4(albedo.rgb * diffuse, 1.0f);
}