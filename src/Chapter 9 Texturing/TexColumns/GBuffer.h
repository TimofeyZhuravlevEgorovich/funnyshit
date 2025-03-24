#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>

class GBuffer {
public:
    void Initialize(ID3D12Device* device, int width, int height);
    void Release();  // Добавляем метод очистки ресурсов

    ID3D12Resource* GetAlbedoBuffer() { return mAlbedoBuffer.Get(); }
    ID3D12Resource* GetNormalBuffer() { return mNormalBuffer.Get(); }
    ID3D12Resource* GetMaterialBuffer() { return mMaterialBuffer.Get(); }
    ID3D12Resource* GetDepthBuffer() { return mDepthBuffer.Get(); }

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GetRenderTargetViews() const;
    DXGI_FORMAT GetFormat(int index) const;  // Новый метод для получения формата
    int GetRenderTargetCount() const;

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRTVHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDSVHeap;

    Microsoft::WRL::ComPtr<ID3D12Resource> mAlbedoBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> mNormalBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> mMaterialBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthBuffer;

    D3D12_CPU_DESCRIPTOR_HANDLE mAlbedoRTV = {};
    D3D12_CPU_DESCRIPTOR_HANDLE mNormalRTV = {};
    D3D12_CPU_DESCRIPTOR_HANDLE mMaterialRTV = {};
    D3D12_CPU_DESCRIPTOR_HANDLE mDepthDSV = {};

    std::vector<DXGI_FORMAT> mRenderTargetFormats = {
        DXGI_FORMAT_R8G8B8A8_UNORM,  // Albedo
        DXGI_FORMAT_R16G16B16A16_FLOAT,  // Normal
        DXGI_FORMAT_R8G8B8A8_UNORM  // Material
    };
};