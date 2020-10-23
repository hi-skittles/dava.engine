#include "../Common/rhi_Private.h"
    #include "../Common/rhi_Pool.h"
    #include "rhi_Metal.h"

    #include "Debug/DVAssert.h"
    #include "Logger/Logger.h"
using DAVA::Logger;

    #include "_metal.h"

#if !(TARGET_IPHONE_SIMULATOR == 1)

namespace rhi
{
//==============================================================================

struct
DepthStencilStateMetal_t
{
    uint32 stencilEnabled : 1;
    id<MTLDepthStencilState> uid;
    uint8 stencilRefValue;
};

typedef ResourcePool<DepthStencilStateMetal_t, RESOURCE_DEPTHSTENCIL_STATE, DepthStencilState::Descriptor, false> DepthStencilStateMetalPool;
RHI_IMPL_POOL(DepthStencilStateMetal_t, RESOURCE_DEPTHSTENCIL_STATE, DepthStencilState::Descriptor, false);

static MTLStencilOperation
_StencilOp(StencilOperation op)
{
    MTLStencilOperation s = MTLStencilOperationKeep;

    switch (op)
    {
    case STENCILOP_KEEP:
        s = MTLStencilOperationKeep;
        break;
    case STENCILOP_ZERO:
        s = MTLStencilOperationZero;
        break;
    case STENCILOP_REPLACE:
        s = MTLStencilOperationReplace;
        break;
    case STENCILOP_INVERT:
        s = MTLStencilOperationInvert;
        break;
    case STENCILOP_INCREMENT_CLAMP:
        s = MTLStencilOperationIncrementClamp;
        break;
    case STENCILOP_DECREMENT_CLAMP:
        s = MTLStencilOperationDecrementClamp;
        break;
    case STENCILOP_INCREMENT_WRAP:
        s = MTLStencilOperationIncrementWrap;
        break;
    case STENCILOP_DECREMENT_WRAP:
        s = MTLStencilOperationDecrementWrap;
        break;
    }

    return s;
}

static MTLCompareFunction
_CmpFunc(CmpFunc func)
{
    MTLCompareFunction f = MTLCompareFunctionLessEqual;

    switch (func)
    {
    case CMP_NEVER:
        f = MTLCompareFunctionNever;
        break;
    case CMP_LESS:
        f = MTLCompareFunctionLess;
        break;
    case CMP_EQUAL:
        f = MTLCompareFunctionEqual;
        break;
    case CMP_LESSEQUAL:
        f = MTLCompareFunctionLessEqual;
        break;
    case CMP_GREATER:
        f = MTLCompareFunctionGreater;
        break;
    case CMP_NOTEQUAL:
        f = MTLCompareFunctionNotEqual;
        break;
    case CMP_GREATEREQUAL:
        f = MTLCompareFunctionGreaterEqual;
        break;
    case CMP_ALWAYS:
        f = MTLCompareFunctionAlways;
        break;
    }

    return f;
}

//==============================================================================

static Handle
metal_DepthStencilState_Create(const DepthStencilState::Descriptor& desc)
{
    Handle handle = DepthStencilStateMetalPool::Alloc();
    DepthStencilStateMetal_t* state = DepthStencilStateMetalPool::Get(handle);
    MTLDepthStencilDescriptor* ds_desc = [MTLDepthStencilDescriptor new];
    MTLStencilDescriptor* front_desc = [MTLStencilDescriptor new];
    MTLStencilDescriptor* back_desc = [MTLStencilDescriptor new];

    if (desc.stencilEnabled)
    {
        front_desc.readMask = desc.stencilFront.readMask;
        front_desc.writeMask = desc.stencilFront.writeMask;
        front_desc.stencilFailureOperation = _StencilOp(StencilOperation(desc.stencilFront.failOperation));
        front_desc.depthFailureOperation = _StencilOp(StencilOperation(desc.stencilFront.depthFailOperation));
        front_desc.depthStencilPassOperation = _StencilOp(StencilOperation(desc.stencilFront.depthStencilPassOperation));
        front_desc.stencilCompareFunction = _CmpFunc(CmpFunc(desc.stencilFront.func));

        back_desc.readMask = desc.stencilBack.readMask;
        back_desc.writeMask = desc.stencilBack.writeMask;
        back_desc.stencilFailureOperation = _StencilOp(StencilOperation(desc.stencilBack.failOperation));
        back_desc.depthFailureOperation = _StencilOp(StencilOperation(desc.stencilBack.depthFailOperation));
        back_desc.depthStencilPassOperation = _StencilOp(StencilOperation(desc.stencilBack.depthStencilPassOperation));
        back_desc.stencilCompareFunction = _CmpFunc(CmpFunc(desc.stencilBack.func));

        ds_desc.frontFaceStencil = front_desc;
        ds_desc.backFaceStencil = desc.stencilTwoSided ? back_desc : front_desc;

        state->stencilRefValue = desc.stencilFront.refValue;
    }

    ds_desc.depthWriteEnabled = (desc.depthWriteEnabled) ? YES : NO;
    ds_desc.depthCompareFunction = (desc.depthTestEnabled) ? _CmpFunc(CmpFunc(desc.depthFunc)) : MTLCompareFunctionAlways;

    state->uid = [_Metal_Device newDepthStencilStateWithDescriptor:ds_desc];
    state->stencilEnabled = desc.stencilEnabled;

    [front_desc release];
    [back_desc release];
    [ds_desc release];

    return handle;
}

static void
metal_DepthStencilState_Delete(Handle state)
{
    DepthStencilStateMetal_t* self = DepthStencilStateMetalPool::Get(state);

    if (self)
    {
        self->uid = nil;
    }

    DepthStencilStateMetalPool::Free(state);
}

namespace DepthStencilStateMetal
{
void Init(uint32 maxCount)
{
    DepthStencilStateMetalPool::Reserve(maxCount);
}
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_DepthStencilState_Create = &metal_DepthStencilState_Create;
    dispatch->impl_DepthStencilState_Delete = &metal_DepthStencilState_Delete;
}

void SetToRHI(Handle hstate, id<MTLRenderCommandEncoder> ce)
{
    DepthStencilStateMetal_t* state = DepthStencilStateMetalPool::Get(hstate);

    [ce setDepthStencilState:state->uid];

    if (state->stencilEnabled)
        [ce setStencilReferenceValue:state->stencilRefValue];
}
}

//==============================================================================
} // namespace rhi

#endif //#if !(TARGET_IPHONE_SIMULATOR==1)