// finalPS.hlsl

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

sampler2D gbufferAlbedo : register(s0);
sampler2D gbufferNormal : register(s1);
sampler2D gbufferMaterial : register(s2);
sampler2D lightingTexture : register(s3);

float4 main(PixelInput input) : SV_Target
{
    // Считывание данных из текстур
    float4 albedo = tex2D(gbufferAlbedo, input.texCoord);
    float4 normal = tex2D(gbufferNormal, input.texCoord);
    float4 material = tex2D(gbufferMaterial, input.texCoord);

    // Считывание освещенности
    float4 lighting = tex2D(lightingTexture, input.texCoord);

    // Композитинг: объединение альбедо, освещения и материала
    return albedo * lighting * material;
}
