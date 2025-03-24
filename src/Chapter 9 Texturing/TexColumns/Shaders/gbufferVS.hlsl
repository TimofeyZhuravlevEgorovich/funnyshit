// gbufferVS.hlsl

cbuffer TransformBuffer : register(b0)
{
    matrix WorldViewProj;
    matrix World;
}

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct VertexOutput
{
    float4 position : SV_POSITION; // Заменили SV_Target на SV_POSITION
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float3 worldPosition : TEXCOORD1;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // Преобразуем вершины в экранные координаты
    output.position = mul(float4(input.position, 1.0f), WorldViewProj); // Должен быть SV_POSITION

    // Преобразуем нормали в мировое пространство
    output.worldPosition = mul(float4(input.position, 1.0f), World).xyz;
    output.normal = mul(input.normal, (float3x3) World);
    output.tangent = mul(input.tangent, (float3x3) World);
    output.bitangent = mul(input.bitangent, (float3x3) World);

    // Передаём текстурные координаты
    output.texCoord = input.texCoord;

    return output;
}
