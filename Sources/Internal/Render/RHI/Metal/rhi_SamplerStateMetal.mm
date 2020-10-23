#include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "../rhi_Public.h"
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
SamplerStateMetal_t
{
    uint32 fp_count;
    id<MTLSamplerState> fp_uid[MAX_FRAGMENT_TEXTURE_SAMPLER_COUNT];
    uint32 vp_count;
    id<MTLSamplerState> vp_uid[MAX_VERTEX_TEXTURE_SAMPLER_COUNT];
};

typedef ResourcePool<SamplerStateMetal_t, RESOURCE_SAMPLER_STATE, SamplerState::Descriptor, false> SamplerStateMetalPool;
RHI_IMPL_POOL(SamplerStateMetal_t, RESOURCE_SAMPLER_STATE, SamplerState::Descriptor, false);

static MTLSamplerAddressMode
_AddrMode(TextureAddrMode mode)
{
    MTLSamplerAddressMode m = MTLSamplerAddressModeRepeat;

    switch (mode)
    {
    case TEXADDR_WRAP:
        m = MTLSamplerAddressModeRepeat;
        break;
    case TEXADDR_CLAMP:
        m = MTLSamplerAddressModeClampToEdge;
        break;
    case TEXADDR_MIRROR:
        m = MTLSamplerAddressModeMirrorRepeat;
        break;
    }

    return m;
}

static MTLSamplerMinMagFilter
_TextureFilter(TextureFilter filter)
{
    MTLSamplerMinMagFilter f = MTLSamplerMinMagFilterNearest;

    switch (filter)
    {
    case TEXFILTER_NEAREST:
        f = MTLSamplerMinMagFilterNearest;
        break;
    case TEXFILTER_LINEAR:
        f = MTLSamplerMinMagFilterLinear;
        break;
    }

    return f;
}

static MTLSamplerMipFilter
_TextureMipFilter(TextureMipFilter filter)
{
    MTLSamplerMipFilter f = MTLSamplerMipFilterNearest;

    switch (filter)
    {
    case TEXMIPFILTER_NONE:
        f = MTLSamplerMipFilterNotMipmapped;
        break;
    case TEXMIPFILTER_NEAREST:
        f = MTLSamplerMipFilterNearest;
        break;
    case TEXMIPFILTER_LINEAR:
        f = MTLSamplerMipFilterLinear;
        break;
    }

    return f;
}

//==============================================================================

static Handle
metal_SamplerState_Create(const SamplerState::Descriptor& desc)
{
    Handle handle = SamplerStateMetalPool::Alloc();
    SamplerStateMetal_t* state = SamplerStateMetalPool::Get(handle);
    MTLSamplerDescriptor* s_desc = [MTLSamplerDescriptor new];

    state->fp_count = desc.fragmentSamplerCount;
    for (unsigned s = 0; s != desc.fragmentSamplerCount; ++s)
    {
        DVASSERT(desc.fragmentSampler[s].anisotropyLevel <= DeviceCaps().maxAnisotropy);

        s_desc.sAddressMode = _AddrMode(TextureAddrMode(desc.fragmentSampler[s].addrU));
        s_desc.tAddressMode = _AddrMode(TextureAddrMode(desc.fragmentSampler[s].addrV));
        s_desc.rAddressMode = _AddrMode(TextureAddrMode(desc.fragmentSampler[s].addrW));
        s_desc.minFilter = _TextureFilter(TextureFilter(desc.fragmentSampler[s].minFilter));
        s_desc.magFilter = _TextureFilter(TextureFilter(desc.fragmentSampler[s].magFilter));
        s_desc.mipFilter = _TextureMipFilter(TextureMipFilter(desc.fragmentSampler[s].mipFilter));
        s_desc.lodMinClamp = 0.0f;
        s_desc.lodMaxClamp = FLT_MAX;
        s_desc.maxAnisotropy = desc.fragmentSampler[s].anisotropyLevel;
        s_desc.normalizedCoordinates = YES;

        state->fp_uid[s] = [_Metal_Device newSamplerStateWithDescriptor:s_desc];
    }

    state->vp_count = desc.vertexSamplerCount;
    for (unsigned s = 0; s != desc.vertexSamplerCount; ++s)
    {
        DVASSERT(desc.vertexSampler[s].anisotropyLevel <= DeviceCaps().maxAnisotropy);

        s_desc.sAddressMode = _AddrMode(TextureAddrMode(desc.vertexSampler[s].addrU));
        s_desc.tAddressMode = _AddrMode(TextureAddrMode(desc.vertexSampler[s].addrV));
        s_desc.rAddressMode = _AddrMode(TextureAddrMode(desc.vertexSampler[s].addrW));
        s_desc.minFilter = _TextureFilter(TextureFilter(desc.vertexSampler[s].minFilter));
        s_desc.magFilter = _TextureFilter(TextureFilter(desc.vertexSampler[s].magFilter));
        s_desc.mipFilter = _TextureMipFilter(TextureMipFilter(desc.vertexSampler[s].mipFilter));
        s_desc.lodMinClamp = 0.0f;
        s_desc.lodMaxClamp = FLT_MAX;
        s_desc.maxAnisotropy = desc.vertexSampler[s].anisotropyLevel;
        s_desc.normalizedCoordinates = YES;

        state->vp_uid[s] = [_Metal_Device newSamplerStateWithDescriptor:s_desc];
    }

    [s_desc release];

    return handle;
}

static void
metal_SamplerState_Delete(Handle state)
{
    SamplerStateMetal_t* self = SamplerStateMetalPool::Get(state);

    if (self)
    {
        for (unsigned s = 0; s != self->fp_count; ++s)
        {
            [self->fp_uid[s] release];
            self->fp_uid[s] = nil;
        }

        for (unsigned s = 0; s != self->vp_count; ++s)
        {
            [self->vp_uid[s] release];
            self->vp_uid[s] = nil;
        }
    }

    SamplerStateMetalPool::Free(state);
}

namespace SamplerStateMetal
{
void Init(uint32 maxCount)
{
    SamplerStateMetalPool::Reserve(maxCount);
}
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_SamplerState_Create = &metal_SamplerState_Create;
    dispatch->impl_SamplerState_Delete = &metal_SamplerState_Delete;
}

void SetToRHI(Handle hstate, id<MTLRenderCommandEncoder> ce)
{
    SamplerStateMetal_t* state = SamplerStateMetalPool::Get(hstate);

    for (unsigned s = 0; s != state->fp_count; ++s)
        [ce setFragmentSamplerState:state->fp_uid[s] atIndex:s];

    for (unsigned s = 0; s != state->vp_count; ++s)
        [ce setVertexSamplerState:state->vp_uid[s] atIndex:s];
}
}

//==============================================================================
} // namespace rhi
#endif //#if !(TARGET_IPHONE_SIMULATOR==1)