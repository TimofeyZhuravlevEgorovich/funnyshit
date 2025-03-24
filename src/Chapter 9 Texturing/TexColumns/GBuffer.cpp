#include "GBuffer.h"

#include <d3dx12.h>
#include <d3dUtil.h>
#include <DirectXColors.h>
#include <iostream>

void GBuffer::Initialize(ID3D12Device* device, int width, int height)
{
    // Создание RTV Heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 3; // Albedo, Normal, Material
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRTVHeap)));

    // Создание DSV Heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDSVHeap)));

    UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Получение дескрипторов
    mAlbedoRTV = mRTVHeap->GetCPUDescriptorHandleForHeapStart();
    mNormalRTV = { mAlbedoRTV.ptr + rtvDescriptorSize };
    mMaterialRTV = { mAlbedoRTV.ptr + rtvDescriptorSize * 2 };
    mDepthDSV = mDSVHeap->GetCPUDescriptorHandleForHeapStart();

    // Создание ресурсов (Albedo, Normal, Material, Depth)
    D3D12_RESOURCE_DESC albedoDesc = {};
    albedoDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    albedoDesc.Width = width;
    albedoDesc.Height = height;
    albedoDesc.DepthOrArraySize = 1;
    albedoDesc.MipLevels = 1;
    albedoDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    albedoDesc.SampleDesc.Count = 1;
    albedoDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    albedoDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    D3D12_CLEAR_VALUE albedoClear = {};
    albedoClear.Format = albedoDesc.Format;
    albedoClear.Color[0] = 0.0f;
    albedoClear.Color[1] = 0.0f;
    albedoClear.Color[2] = 0.0f;
    albedoClear.Color[3] = 1.0f;

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &albedoDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &albedoClear,
        IID_PPV_ARGS(&mAlbedoBuffer)
    ));

    D3D12_RESOURCE_DESC normalDesc = albedoDesc;
    normalDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;  // Более точный формат для нормалей

    D3D12_CLEAR_VALUE normalClear = {};
    normalClear.Format = normalDesc.Format;
    normalClear.Color[0] = 0.5f; // Среднее значение для нормали (X)
    normalClear.Color[1] = 0.5f; // Среднее значение для нормали (Y)
    normalClear.Color[2] = 1.0f; // Направление вверх (Z)
    normalClear.Color[3] = 1.0f;

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &normalDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &normalClear,
        IID_PPV_ARGS(&mNormalBuffer)
    ));

    // Создание Material Buffer
    D3D12_RESOURCE_DESC materialDesc = albedoDesc;
    materialDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // Подходит для хранения материала

    D3D12_CLEAR_VALUE materialClear = {};
    materialClear.Format = materialDesc.Format;
    materialClear.Color[0] = 1.0f; // Белый цвет материала
    materialClear.Color[1] = 1.0f;
    materialClear.Color[2] = 1.0f;
    materialClear.Color[3] = 1.0f;

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &materialDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &materialClear,
        IID_PPV_ARGS(&mMaterialBuffer)
    ));

    // Создание Depth Buffer
    D3D12_RESOURCE_DESC depthDesc = {};
    depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.DepthOrArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    D3D12_CLEAR_VALUE depthClear = {};
    depthClear.Format = depthDesc.Format;
    depthClear.DepthStencil.Depth = 1.0f;
    depthClear.DepthStencil.Stencil = 0;

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthClear,
        IID_PPV_ARGS(&mDepthBuffer)
    ));

    // Создание RTV и DSV
    device->CreateRenderTargetView(mAlbedoBuffer.Get(), nullptr, mAlbedoRTV);
    std::cout << "malbedortv created\n";
    device->CreateRenderTargetView(mNormalBuffer.Get(), nullptr, mNormalRTV);
    std::cout << "mNormalRTV created\n";
    device->CreateRenderTargetView(mMaterialBuffer.Get(), nullptr, mMaterialRTV);
    std::cout << "mMaterialRTV created\n";
    device->CreateDepthStencilView(mDepthBuffer.Get(), nullptr, mDepthDSV);
    std::cout << "mDepthDSV created\n";
}


std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> GBuffer::GetRenderTargetViews() const {
    return { mAlbedoRTV, mNormalRTV, mMaterialRTV };
}

DXGI_FORMAT GBuffer::GetFormat(int index) const {
    if (index >= 0 && index < mRenderTargetFormats.size()) {
        return mRenderTargetFormats[index];
    }
    return DXGI_FORMAT_UNKNOWN; // Возвращаем неизвестный формат в случае ошибки
}

int GBuffer::GetRenderTargetCount() const {
    return mRenderTargetFormats.size();
}


void GBuffer::Release() {
    mAlbedoBuffer.Reset();
    mNormalBuffer.Reset();
    mMaterialBuffer.Reset();
    mDepthBuffer.Reset();
    mRTVHeap.Reset();
    mDSVHeap.Reset();
}
