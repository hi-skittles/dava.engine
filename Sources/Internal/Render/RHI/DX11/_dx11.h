#pragma once

#include "Base/Platform.h"
#include "Base/BaseObject.h"
#include "Concurrency/Mutex.h"
#include "../Common/rhi_RingBuffer.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Private.h"
#include "../Common/rhi_BackendImpl.h"
#include "../rhi_Type.h"
#include "../rhi_Public.h"

#if defined(__DAVAENGINE_WIN_UAP__)
    #include <DXGI1_3.h>
#elif !defined(_WIN32_WINNT)
    #define _WIN32_WINNT 0x0601
#endif

#include <d3d11_1.h>
#include <dxgi.h>
#include <dxgiformat.h>
#include <wrl/client.h>

namespace rhi
{
struct DX11Command
{
    enum Func : uint32_t
    {
        NOP,

        MAP,
        UNMAP,
        UPDATE_SUBRESOURCE,
        COPY_RESOURCE,
        SYNC_CPU_GPU,

        /*
        * Device commands (invokes _D3D11_Device method)
        */
        QUERY_INTERFACE = 0x1000,
        CREATE_DEFERRED_CONTEXT,

        CREATE_BLEND_STATE,
        CREATE_SAMPLER_STATE,
        CREATE_RASTERIZER_STATE,
        CREATE_DEPTH_STENCIL_STATE,

        CREATE_VERTEX_SHADER,
        CREATE_PIXEL_SHADER,
        CREATE_INPUT_LAYOUT,

        CREATE_QUERY,
        CREATE_BUFFER,

        CREATE_TEXTURE_2D,
        CREATE_RENDER_TARGET_VIEW,
        CREATE_DEPTH_STENCIL_VIEW,
        CREATE_SHADER_RESOURCE_VIEW,

        CHECK_FORMAT_SUPPORT,

        // service values for range checking
        DEVICE_LAST_COMMAND,
        DEVICE_FIRST_COMMAND = QUERY_INTERFACE
    };

    struct Arguments
    {
        uint64 arg[12];
    };

    Func func = Func::NOP;
    HRESULT retval = S_OK;
    Arguments arguments;

    template <class... args>
    DX11Command(Func f, args&&... a)
        : func(f)
        , arguments({ uint64(a)... })
    {
    }
};

#if defined(__DAVAENGINE_WIN_UAP__)
using IDXGISwapChainClass = IDXGISwapChain2;
#else
using IDXGISwapChainClass = IDXGISwapChain1;
#endif

struct DX11Resources
{
    InitParam initParameters;

    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
    Microsoft::WRL::ComPtr<IDXGISwapChainClass> swapChain;
    Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> renderTarget;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> deferredContext;

    DAVA::Mutex deferredContextLock;

    UINT BackBuffersCount = 3;
    DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    D3D_FEATURE_LEVEL usedFeatureLevel = D3D_FEATURE_LEVEL_9_1;

    bool useHardwareCommandBuffers = true;

#if defined(__DAVAENGINE_DEBUG__)
    bool hasDebugLayers = false;
#endif

    ID3D11DeviceContext* ImmediateContext()
    {
        return context.Get();
    }
};

const char* DX11_GetErrorText(HRESULT hr);
DXGI_FORMAT DX11_TextureFormat(TextureFormat format);
D3D11_COMPARISON_FUNC DX11_CmpFunc(CmpFunc func);
D3D11_STENCIL_OP DX11_StencilOp(StencilOperation op);
D3D11_TEXTURE_ADDRESS_MODE DX11_TextureAddrMode(TextureAddrMode mode);
D3D11_FILTER DX11_TextureFilter(TextureFilter min_filter, TextureFilter mag_filter, TextureMipFilter mip_filter, uint32 anisotropy);
D3D11_BLEND DX11_BlendOp(BlendOp op);
}
