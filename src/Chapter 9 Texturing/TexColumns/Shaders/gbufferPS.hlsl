// gbufferPS.hlsl

Texture2D albedoTexture : register(t0); // Альбедо текстура
SamplerState samplerState : register(s0);

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

// Входные данные из G-буфера
struct GBuffer
{
    float4 albedo : SV_Target0; // Альбедо
    float4 normal : SV_Target1; // Нормаль
    float4 material : SV_Target2; // Материал
    float depth : SV_Depth; // Глубина
};

GBuffer main(PixelInput input)
{
    GBuffer output;

    // Семплируем текстуру правильно
    output.albedo = albedoTexture.Sample(samplerState, input.texCoord);

    // Нормаль (с нормализацией)
    output.normal = float4(normalize(input.normal), 1.0f);

    // Материал (например, белый, если статично)
    output.material = float4(1.0f, 1.0f, 1.0f, 1.0f);

    // Глубина (нормализуем)
    output.depth = input.position.z / input.position.w;

    return output;
}
