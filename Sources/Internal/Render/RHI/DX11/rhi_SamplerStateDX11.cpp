#include "rhi_DX11.h"

namespace rhi
{
struct SamplerStateDX11_t
{
    uint32 fragmentSamplerCount = 0;
    uint32 vertexSamplerCount = 0;
    ID3D11SamplerState* fragmentSampler[MAX_FRAGMENT_TEXTURE_SAMPLER_COUNT];
    ID3D11SamplerState* vertexSampler[MAX_FRAGMENT_TEXTURE_SAMPLER_COUNT];
};
using SamplerStateDX11Pool = ResourcePool<SamplerStateDX11_t, RESOURCE_SAMPLER_STATE, SamplerState::Descriptor, false>;
RHI_IMPL_POOL(SamplerStateDX11_t, RESOURCE_SAMPLER_STATE, SamplerState::Descriptor, false);

static void dx11_SamplerState_Delete(Handle hstate)
{
    SamplerStateDX11_t* state = SamplerStateDX11Pool::Get(hstate);
    for (ID3D11SamplerState* smp : state->fragmentSampler)
        DAVA::SafeRelease(smp);
    for (ID3D11SamplerState* smp : state->vertexSampler)
        DAVA::SafeRelease(smp);
    SamplerStateDX11Pool::Free(hstate);
}

D3D11_FILTER DX11_SamplerToFilter(const SamplerState::Descriptor::Sampler& smp, UINT MaxAnisotropy)
{
    return DX11_TextureFilter(TextureFilter(smp.minFilter), TextureFilter(smp.magFilter), TextureMipFilter(smp.mipFilter), MaxAnisotropy);
}

static Handle dx11_SamplerState_Create(const SamplerState::Descriptor& desc)
{
    Handle handle = SamplerStateDX11Pool::Alloc();
    SamplerStateDX11_t* state = SamplerStateDX11Pool::Get(handle);
    bool success = true;

    memset(state->fragmentSampler, 0, sizeof(state->fragmentSampler));
    state->fragmentSamplerCount = desc.fragmentSamplerCount;
    for (uint32 s = 0; s != desc.fragmentSamplerCount; ++s)
    {
        D3D11_SAMPLER_DESC s_desc = {};
        s_desc.MaxAnisotropy = desc.fragmentSampler[s].anisotropyLevel;
        s_desc.Filter = DX11_SamplerToFilter(desc.fragmentSampler[s], s_desc.MaxAnisotropy);
        s_desc.AddressU = DX11_TextureAddrMode(TextureAddrMode(desc.fragmentSampler[s].addrU));
        s_desc.AddressV = DX11_TextureAddrMode(TextureAddrMode(desc.fragmentSampler[s].addrV));
        s_desc.AddressW = DX11_TextureAddrMode(TextureAddrMode(desc.fragmentSampler[s].addrW));
        s_desc.MinLOD = -D3D11_FLOAT32_MAX;
        s_desc.MaxLOD = D3D11_FLOAT32_MAX;

        DVASSERT(s_desc.MaxAnisotropy >= 1);
        DVASSERT(s_desc.MaxAnisotropy <= rhi::DeviceCaps().maxAnisotropy);

        if (!DX11DeviceCommand(DX11Command::CREATE_SAMPLER_STATE, &s_desc, state->fragmentSampler + s))
        {
            state->fragmentSampler[s] = nullptr;
            success = false;
        }
    }

    memset(state->vertexSampler, 0, sizeof(state->vertexSampler));
    state->vertexSamplerCount = desc.vertexSamplerCount;
    for (uint32 s = 0; s != desc.vertexSamplerCount; ++s)
    {
        D3D11_SAMPLER_DESC s_desc = {};
        s_desc.MaxAnisotropy = desc.vertexSampler[s].anisotropyLevel;
        s_desc.Filter = DX11_SamplerToFilter(desc.vertexSampler[s], s_desc.MaxAnisotropy);
        s_desc.AddressU = DX11_TextureAddrMode(TextureAddrMode(desc.vertexSampler[s].addrU));
        s_desc.AddressV = DX11_TextureAddrMode(TextureAddrMode(desc.vertexSampler[s].addrV));
        s_desc.AddressW = DX11_TextureAddrMode(TextureAddrMode(desc.vertexSampler[s].addrW));
        s_desc.MinLOD = -D3D11_FLOAT32_MAX;
        s_desc.MaxLOD = D3D11_FLOAT32_MAX;

        DVASSERT(s_desc.MaxAnisotropy >= 1);
        DVASSERT(s_desc.MaxAnisotropy <= rhi::DeviceCaps().maxAnisotropy);

        if (!DX11DeviceCommand(DX11Command::CREATE_SAMPLER_STATE, &s_desc, state->vertexSampler + s))
        {
            state->vertexSampler[s] = nullptr;
            success = false;
        }
    }

    if (!success)
    {
        dx11_SamplerState_Delete(handle);
        handle = InvalidHandle;
    }

    return handle;
}

void SamplerStateDX11::Init(uint32 maxCount)
{
    SamplerStateDX11Pool::Reserve(maxCount);
}
void SamplerStateDX11::SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_SamplerState_Create = &dx11_SamplerState_Create;
    dispatch->impl_SamplerState_Delete = &dx11_SamplerState_Delete;
}

void SamplerStateDX11::SetToRHI(Handle hstate, ID3D11DeviceContext* context)
{
    SamplerStateDX11_t* state = SamplerStateDX11Pool::Get(hstate);
    context->PSSetSamplers(0, state->fragmentSamplerCount, state->fragmentSampler);
    if (state->vertexSamplerCount && DeviceCaps().isVertexTextureUnitsSupported)
    {
        context->VSSetSamplers(0, state->vertexSamplerCount, state->vertexSampler);
    }
}
}
