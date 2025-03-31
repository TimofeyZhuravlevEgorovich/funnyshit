#include "RenderingSystem.h"

void RenderingSystem::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>>& Shaders, std::vector<D3D12_INPUT_ELEMENT_DESC>& InputLayout)
{
    CD3DX12_DESCRIPTOR_RANGE srvTable[2];
    srvTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // t0
    srvTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0); // t1

    CD3DX12_ROOT_PARAMETER slotRootParameter[5];

    slotRootParameter[0].InitAsDescriptorTable(1, &srvTable[0], D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvTable[1], D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[2].InitAsConstantBufferView(0); // register b0
    slotRootParameter[3].InitAsConstantBufferView(1); // register b1
    slotRootParameter[4].InitAsConstantBufferView(2); // register b2


    // ��������� �������
    static const CD3DX12_STATIC_SAMPLER_DESC sampler(
        0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);
    if (FAILED(hr)) {
        OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

    // ������� PipelineState ��� GBuffer
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gbufferPsoDesc = {};
    gbufferPsoDesc.pRootSignature = rootSignature.Get();
    gbufferPsoDesc.InputLayout = { InputLayout.data(), (UINT)InputLayout.size() };
    gbufferPsoDesc.VS = { reinterpret_cast<BYTE*>(Shaders["gbufferVS"]->GetBufferPointer()), Shaders["gbufferVS"]->GetBufferSize() };
    gbufferPsoDesc.PS = { reinterpret_cast<BYTE*>(Shaders["gbufferPS"]->GetBufferPointer()), Shaders["gbufferPS"]->GetBufferSize() };
    gbufferPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    gbufferPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    gbufferPsoDesc.NumRenderTargets = 3; // ���� � ���� 3 ������-�������
    gbufferPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // Diffuse
    gbufferPsoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT; // Normals
    gbufferPsoDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM; // Depth
    gbufferPsoDesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN; // ���������� ������� ������ ���� UNKNOWN
    gbufferPsoDesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;

    // ��������� ��������� RTV ������� UNKNOWN
    for (int i = 3; i < 8; i++) {
        gbufferPsoDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
    }

    // ������ depth-������
    gbufferPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    gbufferPsoDesc.SampleMask = UINT_MAX;
    gbufferPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    gbufferPsoDesc.SampleDesc.Count = 1;
    gbufferPsoDesc.SampleDesc.Quality = 0;
    device->CreateGraphicsPipelineState(&gbufferPsoDesc, IID_PPV_ARGS(&gbufferPipelineState));

    // ������� PipelineState ��� ���������
    D3D12_GRAPHICS_PIPELINE_STATE_DESC lightingPsoDesc = {};
    lightingPsoDesc.pRootSignature = rootSignature.Get();
    lightingPsoDesc.InputLayout = { InputLayout.data(), (UINT)InputLayout.size() };
    lightingPsoDesc.VS = { reinterpret_cast<BYTE*>(Shaders["fullscreenVS"]->GetBufferPointer()), Shaders["fullscreenVS"]->GetBufferSize() };
    lightingPsoDesc.PS = { reinterpret_cast<BYTE*>(Shaders["lightingPS"]->GetBufferPointer()), Shaders["lightingPS"]->GetBufferSize() };
    lightingPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    lightingPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    lightingPsoDesc.NumRenderTargets = 1;
    lightingPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // BackBuffer ������
    lightingPsoDesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN; // �����������!
    lightingPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    lightingPsoDesc.SampleMask = UINT_MAX;
    device->CreateGraphicsPipelineState(&lightingPsoDesc, IID_PPV_ARGS(&lightingPipelineState));

    // ��������� ���������� ��������
    indexCount = 3;
}

void RenderingSystem::RenderGBuffer(ID3D12GraphicsCommandList* cmdList, GBuffer& gBuffer)
{
    cmdList->SetPipelineState(gbufferPipelineState.Get());
    cmdList->SetGraphicsRootSignature(rootSignature.Get());

    // ����������� RTV ��� GBuffer
    auto rtvHandles = gBuffer.GetRenderTargetViews();
    cmdList->OMSetRenderTargets(static_cast<UINT>(rtvHandles.size()), rtvHandles.data(), FALSE, nullptr);

    // ������ ���� �����
    cmdList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
}

void RenderingSystem::RenderLighting(ID3D12GraphicsCommandList* cmdList, GBuffer& gBuffer, ID3D12Resource* backBuffer)
{
    D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV; // ������ RTV ���������� backBuffer
    cmdList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);

    cmdList->SetPipelineState(lightingPipelineState.Get());
    cmdList->SetGraphicsRootSignature(rootSignature.Get());

    // ������ full-screen quad ��� ���������
    cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0); // ������ �������� ����
}
