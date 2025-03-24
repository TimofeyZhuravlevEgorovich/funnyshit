// lightingPS.hlsl

#include "LightingUtil.hlsl"

// Структуры данных для G-буфера
struct GBuffer
{
    float4 albedo : SV_Target0; // Альбедо
    float4 normal : SV_Target1; // Нормаль
    float4 material : SV_Target2; // Материал (например, цвет, характеристики)
};

struct PixelInput
{
    float4 position : SV_POSITION; // Позиция пикселя на экране
    float2 texCoord : TEXCOORD; // Текстурные координаты
};

// Массив источников света
uniform Light gLights[MaxLights];

// Обработчик пикселя
float4 main(PixelInput input) : SV_Target
{
    // Получаем данные из G-буфера
    GBuffer gBufferData;
    gBufferData.albedo = tex2D(albedoTexture, input.texCoord);
    gBufferData.normal = tex2D(normalTexture, input.texCoord);
    gBufferData.material = tex2D(materialTexture, input.texCoord);
    
    // Вектора
    float3 normal = normalize(gBufferData.normal.xyz);
    float3 position = input.position.xyz;
    float3 toEye = normalize(cameraPosition - position); // Камера находится в (cameraPosition)
    
    // Применяем освещение
    float3 shadowFactor = 1.0f; // Можно добавить сюда расчёт для теней, если необходимо
    float4 lightingColor = ComputeLighting(gLights, gBufferData.material, position, normal, toEye, shadowFactor);
    
    // Возвращаем результат освещения
    return float4(lightingColor.rgb * gBufferData.albedo.rgb, 1.0f);
}
