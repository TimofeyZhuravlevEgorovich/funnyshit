#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
#include "GBuffer.h"
#include <filesystem>
#include <unordered_map>

class RenderingSystem {
public:
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>>& Shaders, std::vector<D3D12_INPUT_ELEMENT_DESC>& InputLayout);
    void RenderGBuffer(ID3D12GraphicsCommandList* cmdList, GBuffer& gBuffer);
    void RenderLighting(ID3D12GraphicsCommandList* cmdList, GBuffer& gBuffer, ID3D12Resource* backBuffer);
    ID3D12RootSignature* GetRootSignature() { return rootSignature.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> gbufferPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> lightingPipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    UINT indexCount = 0; // Количество индексов для рендера

};
