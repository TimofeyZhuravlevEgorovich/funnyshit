// fullscreenVS.hlsl

struct VertexInput
{
    float2 position : POSITION; // Позиция вершины
    float2 texCoord : TEXCOORD; // Текстурные координаты
};

struct PixelInput
{
    float4 position : SV_POSITION; // Проекция позиции
    float2 texCoord : TEXCOORD;
};

// Треугольник на полный экран
PixelInput main(VertexInput input)
{
    PixelInput output;

    // Преобразование координат
    output.position = float4(input.position, 0.0f, 1.0f);
    output.texCoord = input.texCoord;

    return output;
}
