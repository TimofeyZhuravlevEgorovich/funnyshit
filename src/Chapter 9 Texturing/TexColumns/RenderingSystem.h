#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "GBuffer.h"

class RenderingSystem {
public:
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
    void RenderGBuffer(ID3D12GraphicsCommandList* cmdList, GBuffer& gBuffer);
    void RenderLighting(ID3D12GraphicsCommandList* cmdList, GBuffer& gBuffer, ID3D12Resource* backBuffer);

private:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> gbufferPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> lightingPipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    UINT indexCount = 0; // Количество индексов для рендера
};
