// gbufferVS.hlsl

struct VertexInput
{
    float3 position : POSITION; // ������� �������
    float3 normal : NORMAL; // �������
    float2 texCoord : TEXCOORD; // ���������� ����������
    float3 tangent : TANGENT; // �������
    float3 bitangent : BITANGENT; // ����������
};

struct PixelInput
{
    float4 position : SV_POSITION; // �������� ������� � ������������ ������
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

PixelInput main(VertexInput input)
{
    PixelInput output;

    // �������� �������
    output.position = mul(float4(input.position, 1.0f), WorldViewProj); // WorldViewProj - ������� �������������� (world, view, projection)
    
    // �������� �������, ���������� ���������� � ������ ������
    output.normal = normalize(input.normal);
    output.texCoord = input.texCoord;
    output.tangent = input.tangent;
    output.bitangent = input.bitangent;

    return output;
}
