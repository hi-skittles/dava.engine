#include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "Debug/DVAssert.h"
#include "rhi_DX9.h"
#include "_dx9.h"

namespace rhi
{
//==============================================================================

struct
SamplerStateDX9_t
{
    struct
    sampler_t
    {
        DWORD addrU;
        DWORD addrV;
        DWORD addrW;
        DWORD minFilter;
        DWORD magFilter;
        DWORD mipFilter;
        DWORD anisotropyLevel;
    };

    sampler_t fragmentSampler[MAX_FRAGMENT_TEXTURE_SAMPLER_COUNT];
    uint32 fragmentSamplerCount;

    sampler_t vertexSampler[MAX_VERTEX_TEXTURE_SAMPLER_COUNT];
    uint32 vertexSamplerCount;
};

typedef ResourcePool<SamplerStateDX9_t, RESOURCE_SAMPLER_STATE, SamplerState::Descriptor, false> SamplerStateDX9Pool;
RHI_IMPL_POOL(SamplerStateDX9_t, RESOURCE_SAMPLER_STATE, SamplerState::Descriptor, false);

//------------------------------------------------------------------------------

static DWORD
_AddrModeDX9(TextureAddrMode mode)
{
    DWORD m = D3DTADDRESS_WRAP;

    switch (mode)
    {
    case TEXADDR_WRAP:
        m = D3DTADDRESS_WRAP;
        break;
    case TEXADDR_CLAMP:
        m = D3DTADDRESS_CLAMP;
        break;
    case TEXADDR_MIRROR:
        m = D3DTADDRESS_MIRROR;
        break;
    }

    return m;
}

//------------------------------------------------------------------------------

static DWORD
_TextureFilterDX9(TextureFilter filter, DAVA::uint32 anisotropyLevel)
{
    DWORD f = 0;

    switch (filter)
    {
    case TEXFILTER_NEAREST:
        f = D3DTEXF_POINT;
        break;
    case TEXFILTER_LINEAR:
        f = anisotropyLevel > 1 ? D3DTEXF_ANISOTROPIC : D3DTEXF_LINEAR;
        break;
    default:
        DVASSERT(0, "Invalid TextureFilter value");
    }

    return f;
}

//------------------------------------------------------------------------------

static DWORD
_TextureMipFilterDX9(TextureMipFilter filter, DAVA::uint32 anisotropyLevel)
{
    DWORD f = 0;

    switch (filter)
    {
    case TEXMIPFILTER_NONE:
        f = D3DTEXF_NONE;
        break;
    case TEXMIPFILTER_NEAREST:
        f = D3DTEXF_POINT;
        break;
    case TEXMIPFILTER_LINEAR:
        f = anisotropyLevel > 1 ? D3DTEXF_ANISOTROPIC : D3DTEXF_LINEAR;
        break;
    default:
        DVASSERT(0, "Invalid TextureMipFilter value");
    }

    return f;
}

//==============================================================================

static Handle
dx9_SamplerState_Create(const SamplerState::Descriptor& desc)
{
    Handle handle = SamplerStateDX9Pool::Alloc();
    SamplerStateDX9_t* state = SamplerStateDX9Pool::Get(handle);

    state->fragmentSamplerCount = desc.fragmentSamplerCount;
    for (unsigned i = 0; i != desc.fragmentSamplerCount; ++i)
    {
        uint32 anisotropyLevel = desc.fragmentSampler[i].anisotropyLevel;
        DVASSERT(anisotropyLevel >= 1);
        DVASSERT(anisotropyLevel <= rhi::DeviceCaps().maxAnisotropy);

        state->fragmentSampler[i].addrU = _AddrModeDX9(TextureAddrMode(desc.fragmentSampler[i].addrU));
        state->fragmentSampler[i].addrV = _AddrModeDX9(TextureAddrMode(desc.fragmentSampler[i].addrV));
        state->fragmentSampler[i].addrW = _AddrModeDX9(TextureAddrMode(desc.fragmentSampler[i].addrW));
        state->fragmentSampler[i].minFilter = _TextureFilterDX9(TextureFilter(desc.fragmentSampler[i].minFilter), anisotropyLevel);
        state->fragmentSampler[i].magFilter = _TextureFilterDX9(TextureFilter(desc.fragmentSampler[i].magFilter), anisotropyLevel);
        state->fragmentSampler[i].mipFilter = _TextureMipFilterDX9(TextureMipFilter(desc.fragmentSampler[i].mipFilter), anisotropyLevel);
        state->fragmentSampler[i].anisotropyLevel = anisotropyLevel;
    }

    state->vertexSamplerCount = desc.vertexSamplerCount;
    for (unsigned i = 0; i != desc.vertexSamplerCount; ++i)
    {
        uint32 anisotropyLevel = desc.vertexSampler[i].anisotropyLevel;
        DVASSERT(anisotropyLevel >= 1);
        DVASSERT(anisotropyLevel <= rhi::DeviceCaps().maxAnisotropy);

        state->vertexSampler[i].addrU = _AddrModeDX9(TextureAddrMode(desc.vertexSampler[i].addrU));
        state->vertexSampler[i].addrV = _AddrModeDX9(TextureAddrMode(desc.vertexSampler[i].addrV));
        state->vertexSampler[i].addrW = _AddrModeDX9(TextureAddrMode(desc.vertexSampler[i].addrW));
        state->vertexSampler[i].minFilter = _TextureFilterDX9(TextureFilter(desc.vertexSampler[i].minFilter), anisotropyLevel);
        state->vertexSampler[i].magFilter = _TextureFilterDX9(TextureFilter(desc.vertexSampler[i].magFilter), anisotropyLevel);
        state->vertexSampler[i].mipFilter = _TextureMipFilterDX9(TextureMipFilter(desc.vertexSampler[i].mipFilter), anisotropyLevel);
        state->vertexSampler[i].anisotropyLevel = anisotropyLevel;
    }

    return handle;
}

//------------------------------------------------------------------------------

static void
dx9_SamplerState_Delete(Handle state)
{
    SamplerStateDX9Pool::Free(state);
}

//==============================================================================

namespace SamplerStateDX9
{
void Init(uint32 maxCount)
{
    SamplerStateDX9Pool::Reserve(maxCount);
}
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_SamplerState_Create = &dx9_SamplerState_Create;
    dispatch->impl_SamplerState_Delete = &dx9_SamplerState_Delete;
}

void SetToRHI(Handle hstate)
{
    SamplerStateDX9_t* state = SamplerStateDX9Pool::Get(hstate);

    for (unsigned i = 0; i != state->fragmentSamplerCount; ++i)
    {
        _D3D9_Device->SetSamplerState(i, D3DSAMP_ADDRESSU, state->fragmentSampler[i].addrU);
        _D3D9_Device->SetSamplerState(i, D3DSAMP_ADDRESSV, state->fragmentSampler[i].addrV);
        _D3D9_Device->SetSamplerState(i, D3DSAMP_ADDRESSW, state->fragmentSampler[i].addrW);
        _D3D9_Device->SetSamplerState(i, D3DSAMP_MINFILTER, state->fragmentSampler[i].minFilter);
        _D3D9_Device->SetSamplerState(i, D3DSAMP_MAGFILTER, state->fragmentSampler[i].magFilter);
        _D3D9_Device->SetSamplerState(i, D3DSAMP_MIPFILTER, state->fragmentSampler[i].mipFilter);

        if (rhi::DeviceCaps().isAnisotropicFilteringSupported())
        {
            _D3D9_Device->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, state->fragmentSampler[i].anisotropyLevel);
        }
    }

    for (unsigned i = 0; i != state->vertexSamplerCount; ++i)
    {
        DWORD sampler = D3DDMAPSAMPLER + 1 + i;
        _D3D9_Device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, state->vertexSampler[i].addrU);
        _D3D9_Device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, state->vertexSampler[i].addrV);
        _D3D9_Device->SetSamplerState(sampler, D3DSAMP_ADDRESSW, state->vertexSampler[i].addrW);
        _D3D9_Device->SetSamplerState(sampler, D3DSAMP_MINFILTER, state->vertexSampler[i].minFilter);
        _D3D9_Device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, state->vertexSampler[i].magFilter);
        _D3D9_Device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, state->vertexSampler[i].mipFilter);

        if (rhi::DeviceCaps().isAnisotropicFilteringSupported())
        {
            _D3D9_Device->SetSamplerState(sampler, D3DSAMP_MAXANISOTROPY, state->vertexSampler[i].anisotropyLevel);
        }
    }
}
}

//==============================================================================
} // namespace rhi
