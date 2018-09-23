#pragma once

#include <wrl.h>
#include <d3d12.h>
#include "d3dx12.h"

using namespace Microsoft::WRL;

class ResourceBuffer {

    ComPtr<ID3D12Resource>    _defaultBuffer;
    ComPtr<ID3D12Resource>    _uploadBuffer;

public :
    //Buffer
    ResourceBuffer(const void* initData,
        UINT byteSize,
        ComPtr<ID3D12GraphicsCommandList>& cmdList,
        ComPtr<ID3D12Device>& device);

    //Texture2D
    ResourceBuffer(const void* initData,
        UINT byteSize, UINT width, UINT height,
        ComPtr<ID3D12GraphicsCommandList>& cmdList,
        ComPtr<ID3D12Device>& device);

    D3D12_GPU_VIRTUAL_ADDRESS getGPUAddress();
    D3D12_RESOURCE_DESC       getDescriptor();
    ID3D12Resource*           getResource();

};