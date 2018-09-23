#pragma once

#include <d3d12.h>
#include "d3dx12.h"
#include <D3Dcompiler.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "DXGI.h"
#include "D3d12SDKLayers.h"
#include <vector>
#include "ConstantBuffer.h"
#include "AssetTexture.h"
#include "ResourceBuffer.h"
#include "PipelineShader.h"
#include "PresentTarget.h"

#pragma comment(lib, "D3D12.lib") 
#pragma comment(lib, "dxgi.lib") 
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib") 

// These settings can be adjusted as needed
#define MAX_SRVS 128
#define CMD_LIST_NUM 2
using namespace Microsoft::WRL;


class DXLayer {
public:
    DXLayer(HWND wnd, DWORD width, DWORD height, int cmdShow);
    ~DXLayer();

    void run();

private:

    void _render();
    void _flushCommandListAndWait(ID3D12PipelineState *pNextPSO, bool waitOnLatest);
    void _buildTriangle();

    ConstantBuffer*                                _mvpConstBuff;
    PresentTarget*                                 _presentTarget;
    PipelineShader*                                _pipelineShader;
    ResourceBuffer*                                _vertexBuffer;
    ResourceBuffer*                                _indexBuffer;
    AssetTexture*                                  _textureAsset;
    ComPtr<ID3D12Device>                           _device;
    ComPtr<ID3D12CommandAllocator>                 _cmdAllocator;
    ComPtr<ID3D12GraphicsCommandList>              _cmdLists[CMD_LIST_NUM];
    ComPtr<ID3D12Fence>                            _cmdListFence;
    ComPtr<ID3D12CommandQueue>                     _cmdQueue;
    D3D12_INDEX_BUFFER_VIEW                        _ibv;
    D3D12_VERTEX_BUFFER_VIEW                       _vbv;
    HWND                                           _window;
    HANDLE                                         _event;
    int                                            _cmdShow;
    int                                            _cmdListIndex;
    int                                            _nextFenceValue;
    int                                            _cmdListFenceValues[CMD_LIST_NUM];
    const DXGI_FORMAT                              _rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
};