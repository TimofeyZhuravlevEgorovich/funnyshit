// gbufferVS.hlsl

struct VertexInput
{
    float3 position : POSITION; // Позиция вершины
    float3 normal : NORMAL; // Нормаль
    float2 texCoord : TEXCOORD; // Текстурные координаты
    float3 tangent : TANGENT; // Тангент
    float3 bitangent : BITANGENT; // Битангенты
};

struct PixelInput
{
    float4 position : SV_POSITION; // Проекция позиции в пространство экрана
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

PixelInput main(VertexInput input)
{
    PixelInput output;

    // Передаем позицию
    output.position = mul(float4(input.position, 1.0f), WorldViewProj); // WorldViewProj - матрица преобразования (world, view, projection)
    
    // Передаем нормаль, текстурные координаты и другие данные
    output.normal = normalize(input.normal);
    output.texCoord = input.texCoord;
    output.tangent = input.tangent;
    output.bitangent = input.bitangent;

    return output;
}
