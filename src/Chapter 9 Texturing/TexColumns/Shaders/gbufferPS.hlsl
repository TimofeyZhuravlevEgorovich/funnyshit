// gbufferPS.hlsl

Texture2D albedoTexture : register(t0);
SamplerState samplerState : register(s0);

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

// �������� ������ G-������
struct GBufferOutput
{
    float4 albedo : SV_Target0; // �������
    float4 normal : SV_Target1; // �������
    float4 material : SV_Target2; // ��������
    float depth : SV_Depth; // �������
};

GBufferOutput main(PixelInput input)
{
    GBufferOutput output;

    output.albedo = albedoTexture.Sample(samplerState, input.texCoord);
    output.normal = float4(normalize(input.normal), 1.0f);
    output.material = float4(1.0f, 1.0f, 1.0f, 1.0f);

    // ��������� ����������� �������
    output.depth = input.position.z / input.position.w;

    return output;
}