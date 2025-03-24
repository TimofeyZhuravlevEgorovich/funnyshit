#include "RenderingSystem.h"

#include "RenderingSystem.h"

void RenderingSystem::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    // ����� ������� Root Signature (������� �� ����� ��������� ��������)
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 0;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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
    gbufferPsoDesc.VS = { /* ������� ����. ������ */ };
    gbufferPsoDesc.PS = { /* ������� ����. ������ */ };
    gbufferPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // ������ G-������
    gbufferPsoDesc.SampleMask = UINT_MAX;
    gbufferPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    device->CreateGraphicsPipelineState(&gbufferPsoDesc, IID_PPV_ARGS(&gbufferPipelineState));

    // ������� PipelineState ��� ���������
    D3D12_GRAPHICS_PIPELINE_STATE_DESC lightingPsoDesc = {};
    lightingPsoDesc.pRootSignature = rootSignature.Get();
    lightingPsoDesc.VS = { /* ������� ����. ������ */ };
    lightingPsoDesc.PS = { /* ������� ����. ������ ��������� */ };
    lightingPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    device->CreateGraphicsPipelineState(&lightingPsoDesc, IID_PPV_ARGS(&lightingPipelineState));

    // ��������� ���������� ��������
    indexCount = 52;
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
