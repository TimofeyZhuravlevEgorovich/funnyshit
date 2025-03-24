// lightningPS.hlsl

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

// Выходное значение — цвет пикселя
float4 main(PixelInput input) : SV_Target
{
    // Семплируем данные из текстур
    float4 albedo = albedoTexture.Sample(samplerState, input.texCoord);
    float4 normalData = normalTexture.Sample(samplerState, input.texCoord);
    float4 material = materialTexture.Sample(samplerState, input.texCoord);

    // Восстанавливаем нормаль
    float3 normal = normalize(normalData.xyz * 2.0f - 1.0f);

    // Вектор к камере
    float3 viewDir = normalize(cameraPosition - input.worldPosition);

    // Простая ламбертовская модель освещения (временно)
    float3 lightDir = normalize(float3(0.0f, 1.0f, -1.0f)); // Направление света
    float diffuse = max(dot(normal, lightDir), 0.0f);

    // Итоговый цвет
    return float4(albedo.rgb * diffuse, 1.0f);
}
