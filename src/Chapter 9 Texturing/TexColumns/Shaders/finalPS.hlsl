// finalPS.hlsl

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D gbufferAlbedo : register(t0);
Texture2D gbufferNormal : register(t1);
Texture2D gbufferMaterial : register(t2);
Texture2D lightingTexture : register(t3);

SamplerState samplerState : register(s0);

float4 main(PixelInput input) : SV_Target
{
    float4 albedo = gbufferAlbedo.Sample(samplerState, input.texCoord);
    float4 normal = gbufferNormal.Sample(samplerState, input.texCoord);
    float4 material = gbufferMaterial.Sample(samplerState, input.texCoord);
    float4 lighting = lightingTexture.Sample(samplerState, input.texCoord);

    return albedo * lighting * material;
}