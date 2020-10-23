#include "rhi_DX11.h"

namespace rhi
{
struct DepthStencilStateDX11_t
{
    ID3D11DepthStencilState* state = nullptr;
    UINT stencilRef = 0;
};
using DepthStencilStateDX11Pool = ResourcePool<DepthStencilStateDX11_t, RESOURCE_DEPTHSTENCIL_STATE, DepthStencilState::Descriptor, false>;
RHI_IMPL_POOL(DepthStencilStateDX11_t, RESOURCE_DEPTHSTENCIL_STATE, DepthStencilState::Descriptor, false);

static Handle dx11_DepthStencilState_Create(const DepthStencilState::Descriptor& desc)
{
    D3D11_DEPTH_STENCIL_DESC ds_desc = {};
    ds_desc.DepthEnable = desc.depthTestEnabled;
    ds_desc.DepthWriteMask = (desc.depthWriteEnabled) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    ds_desc.DepthFunc = DX11_CmpFunc(CmpFunc(desc.depthFunc));
    ds_desc.StencilEnable = (desc.stencilEnabled) ? TRUE : FALSE;
    ds_desc.StencilReadMask = desc.stencilFront.readMask;
    ds_desc.StencilWriteMask = desc.stencilFront.writeMask;
    ds_desc.FrontFace.StencilFailOp = DX11_StencilOp(StencilOperation(desc.stencilFront.failOperation));
    ds_desc.FrontFace.StencilDepthFailOp = DX11_StencilOp(StencilOperation(desc.stencilFront.depthFailOperation));
    ds_desc.FrontFace.StencilPassOp = DX11_StencilOp(StencilOperation(desc.stencilFront.depthStencilPassOperation));
    ds_desc.FrontFace.StencilFunc = DX11_CmpFunc(CmpFunc(desc.stencilFront.func));
    ds_desc.BackFace.StencilFailOp = DX11_StencilOp(StencilOperation(desc.stencilBack.failOperation));
    ds_desc.BackFace.StencilDepthFailOp = DX11_StencilOp(StencilOperation(desc.stencilBack.depthFailOperation));
    ds_desc.BackFace.StencilPassOp = DX11_StencilOp(StencilOperation(desc.stencilBack.depthStencilPassOperation));
    ds_desc.BackFace.StencilFunc = DX11_CmpFunc(CmpFunc(desc.stencilBack.func));

    Handle handle = InvalidHandle;

    ID3D11DepthStencilState* localState = nullptr;
    bool commandExecuted = DX11DeviceCommand(DX11Command::CREATE_DEPTH_STENCIL_STATE, &ds_desc, &localState);
    if (commandExecuted && (localState != nullptr))
    {
        handle = DepthStencilStateDX11Pool::Alloc();
        DepthStencilStateDX11_t* state = DepthStencilStateDX11Pool::Get(handle);
        state->stencilRef = desc.stencilFront.refValue;
        state->state = localState;
    }
    return handle;
}

static void dx11_DepthStencilState_Delete(Handle hstate)
{
    DepthStencilStateDX11_t* state = DepthStencilStateDX11Pool::Get(hstate);
    DAVA::SafeRelease(state->state);
    DepthStencilStateDX11Pool::Free(hstate);
}

void DepthStencilStateDX11::Init(uint32 maxCount)
{
    DepthStencilStateDX11Pool::Reserve(maxCount);
}

void DepthStencilStateDX11::SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_DepthStencilState_Create = &dx11_DepthStencilState_Create;
    dispatch->impl_DepthStencilState_Delete = &dx11_DepthStencilState_Delete;
}

void DepthStencilStateDX11::SetToRHI(Handle hstate, ID3D11DeviceContext* context)
{
    DepthStencilStateDX11_t* state = DepthStencilStateDX11Pool::Get(hstate);
    context->OMSetDepthStencilState(state->state, state->stencilRef);
}
}
