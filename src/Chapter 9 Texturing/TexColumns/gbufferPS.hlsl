// gbufferPS.hlsl

SamplerState textureSampler : register(s0); // Например, если это сэмплер текстуры

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
    float4 material : SV_Target2; // Материал (например, цвет/характеристики материала)
    float depth : SV_Depth; // Глубина (если нужно)
};

GBuffer main(PixelInput input)
{
    GBuffer output;

    // Альбедо (например, текстурируем по координатам)
    output.albedo = tex2D(textureSampler, input.texCoord); // textureSampler — текстурный сэмплер

    // Нормаль (можно рассчитывать через нормализацию)
    output.normal = float4(input.normal, 1.0f); // Преобразуем в вектор цвета
    output.material = float4(1.0f, 1.0f, 1.0f, 1.0f); // Материал (например, белый, если статично)

    // Глубина
    output.depth = input.position.z;

    return output;
}
