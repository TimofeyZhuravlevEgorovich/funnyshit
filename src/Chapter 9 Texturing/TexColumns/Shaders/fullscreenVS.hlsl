// fullscreenVS.hlsl

struct VertexInput
{
    float2 position : POSITION; // ������� �������
    float2 texCoord : TEXCOORD; // ���������� ����������
};

struct PixelInput
{
    float4 position : SV_POSITION; // �������� �������
    float2 texCoord : TEXCOORD;
};

// ����������� �� ������ �����
PixelInput main(VertexInput input)
{
    PixelInput output;

    // �������������� ���������
    output.position = float4(input.position, 0.0f, 1.0f);
    output.texCoord = input.texCoord;

    return output;
}
