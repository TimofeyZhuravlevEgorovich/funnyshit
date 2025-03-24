// lightingPS.hlsl

#include "LightingUtil.hlsl"

// ��������� ������ ��� G-������
struct GBuffer
{
    float4 albedo : SV_Target0; // �������
    float4 normal : SV_Target1; // �������
    float4 material : SV_Target2; // �������� (��������, ����, ��������������)
};

struct PixelInput
{
    float4 position : SV_POSITION; // ������� ������� �� ������
    float2 texCoord : TEXCOORD; // ���������� ����������
};

// ������ ���������� �����
uniform Light gLights[MaxLights];

// ���������� �������
float4 main(PixelInput input) : SV_Target
{
    // �������� ������ �� G-������
    GBuffer gBufferData;
    gBufferData.albedo = tex2D(albedoTexture, input.texCoord);
    gBufferData.normal = tex2D(normalTexture, input.texCoord);
    gBufferData.material = tex2D(materialTexture, input.texCoord);
    
    // �������
    float3 normal = normalize(gBufferData.normal.xyz);
    float3 position = input.position.xyz;
    float3 toEye = normalize(cameraPosition - position); // ������ ��������� � (cameraPosition)
    
    // ��������� ���������
    float3 shadowFactor = 1.0f; // ����� �������� ���� ������ ��� �����, ���� ����������
    float4 lightingColor = ComputeLighting(gLights, gBufferData.material, position, normal, toEye, shadowFactor);
    
    // ���������� ��������� ���������
    return float4(lightingColor.rgb * gBufferData.albedo.rgb, 1.0f);
}
