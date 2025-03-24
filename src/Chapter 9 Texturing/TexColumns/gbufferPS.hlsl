// gbufferPS.hlsl

SamplerState textureSampler : register(s0); // ��������, ���� ��� ������� ��������

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

// ������� ������ �� G-������
struct GBuffer
{
    float4 albedo : SV_Target0; // �������
    float4 normal : SV_Target1; // �������
    float4 material : SV_Target2; // �������� (��������, ����/�������������� ���������)
    float depth : SV_Depth; // ������� (���� �����)
};

GBuffer main(PixelInput input)
{
    GBuffer output;

    // ������� (��������, ������������ �� �����������)
    output.albedo = tex2D(textureSampler, input.texCoord); // textureSampler � ���������� �������

    // ������� (����� ������������ ����� ������������)
    output.normal = float4(input.normal, 1.0f); // ����������� � ������ �����
    output.material = float4(1.0f, 1.0f, 1.0f, 1.0f); // �������� (��������, �����, ���� ��������)

    // �������
    output.depth = input.position.z;

    return output;
}
