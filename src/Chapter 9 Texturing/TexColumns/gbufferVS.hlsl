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

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld; // ������� �������
    float4x4 gView; // ������� ����
    float4x4 gProj; // ������� ��������
};


PixelInput main(VertexInput input)
{
    PixelInput output;
    
    float4x4 worldViewProj = mul(gWorld, gView); // �������� ������� � ������� ����
    float4x4 worldViewProj = mul(worldViewProj, gProj); // �������� �� ������� ��������
    
    vout.PosH = mul(vout.PosW, worldViewProj);

    // �������� �������
    output.position = mul(float4(input.position, 1.0f), WorldViewProj); // WorldViewProj - ������� �������������� (world, view, projection)
    
    // �������� �������, ���������� ���������� � ������ ������
    output.normal = normalize(input.normal);
    output.texCoord = input.texCoord;
    output.tangent = input.tangent;
    output.bitangent = input.bitangent;

    return output;
}
