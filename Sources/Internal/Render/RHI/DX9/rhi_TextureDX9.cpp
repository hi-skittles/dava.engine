#include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"
#include "../Common/rhi_FormatConversion.h"
#include "rhi_DX9.h"

#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
using DAVA::Logger;

#include "_dx9.h"

namespace rhi
{
//==============================================================================

struct TextureDX9_t : public ResourceImpl<TextureDX9_t, Texture::Descriptor>
{
    bool Create(const Texture::Descriptor& desc, bool forceExecute = false);
    void Destroy(bool forceExecute);

    IDirect3DBaseTexture9* basetex9 = nullptr;
    IDirect3DTexture9* tex9 = nullptr;
    IDirect3DCubeTexture9* cubetex9 = nullptr;
    IDirect3DSurface9* surf9 = nullptr;
    IDirect3DSurface9* cubesurf9[6];
    IDirect3DTexture9* rt_tex9 = nullptr;
    IDirect3DSurface9* rt_surf9 = nullptr;

    uint32 lastUnit = DAVA::InvalidIndex;
    uint32 mappedLevel = 0;
    TextureFace mappedFace = TEXTURE_FACE_NONE;
    void* mappedData = nullptr;
    bool isMapped = false;
};

RHI_IMPL_RESOURCE(TextureDX9_t, Texture::Descriptor)

typedef ResourcePool<TextureDX9_t, RESOURCE_TEXTURE, Texture::Descriptor, true> TextureDX9Pool;
RHI_IMPL_POOL(TextureDX9_t, RESOURCE_TEXTURE, Texture::Descriptor, true);

const TextureFace textureFaces[] =
{
  TEXTURE_FACE_POSITIVE_X, TEXTURE_FACE_NEGATIVE_X,
  TEXTURE_FACE_POSITIVE_Y, TEXTURE_FACE_NEGATIVE_Y,
  TEXTURE_FACE_POSITIVE_Z, TEXTURE_FACE_NEGATIVE_Z
};

D3DCUBEMAP_FACES textureFaceToD3DFace[] =
{
  D3DCUBEMAP_FACE_POSITIVE_X, D3DCUBEMAP_FACE_NEGATIVE_X,
  D3DCUBEMAP_FACE_POSITIVE_Y, D3DCUBEMAP_FACE_NEGATIVE_Y,
  D3DCUBEMAP_FACE_POSITIVE_Z, D3DCUBEMAP_FACE_NEGATIVE_Z
};

//------------------------------------------------------------------------------

bool TextureDX9_t::Create(const Texture::Descriptor& desc, bool forceExecute)
{
    DVASSERT(desc.levelCount);
    bool success = false;
    UpdateCreationDesc(desc);

    memset(cubesurf9, 0, sizeof(cubesurf9));

    D3DFORMAT fmt = DX9_TextureFormat(desc.format);
    DWORD usage = (desc.isRenderTarget) ? D3DUSAGE_RENDERTARGET : 0;
    D3DPOOL pool = (desc.isRenderTarget) ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
    HRESULT hr = E_FAIL;
    bool auto_mip = (desc.autoGenMipmaps) ? true : false;
    unsigned mip_count = desc.isRenderTarget ? 1 : desc.levelCount;

    bool is_depthbuf = (fmt == D3DFMT_D24S8) || (fmt == D3DFMT_D32) || (fmt == D3DFMT_D16);
    if (is_depthbuf)
    {
        pool = D3DPOOL_DEFAULT;
        usage = D3DUSAGE_DEPTHSTENCIL;
    }

    if (desc.sampleCount > 1)
    {
        DX9Command cmds[1] = {};

        if (is_depthbuf)
        {
            cmds[0] = { DX9Command::CREARE_DEPTHSTENCIL_SURFACE, { desc.width, desc.height, uint64(fmt), desc.sampleCount, 0, 0, uint64_t(&surf9), 0 } };
        }
        else
        {
            cmds[0] = { DX9Command::CREATE_RENDER_TARGET, { desc.width, desc.height, uint64(fmt), desc.sampleCount, 0, 0, uint64_t(&surf9), 0 } };
        };
        ExecDX9(cmds, countof(cmds), forceExecute);
        DVASSERT(surf9 != nullptr);

        success = true;
    }
    else
    {
        switch (desc.type)
        {
        case TEXTURE_TYPE_2D:
        {
            DVASSERT(tex9 == nullptr);

            unsigned cmd1_cnt = 1;
            DX9Command cmd1[32] =
            {
              DX9Command::CREATE_TEXTURE, { desc.width, desc.height, mip_count, usage, static_cast<uint64>(fmt), static_cast<uint64>(pool), uint64_t(&tex9), 0 }
            };

            DVASSERT(desc.levelCount <= countof(desc.initialData));
            for (unsigned m = 0; m != desc.levelCount; ++m)
            {
                DX9Command* cmd = cmd1 + cmd1_cnt;

                if (desc.initialData[m])
                {
                    Size2i sz = TextureExtents(Size2i(desc.width, desc.height), m);
                    void* data = desc.initialData[m];
                    uint32 data_sz = TextureSize(desc.format, sz.dx, sz.dy);
                    cmd->func = DX9Command::UPDATE_TEXTURE_LEVEL;
                    cmd->arg[0] = uint64_t(&tex9);
                    cmd->arg[1] = m;
                    cmd->arg[2] = (uint64)(data);
                    cmd->arg[3] = data_sz;
                    cmd->arg[4] = desc.format;
                    ++cmd1_cnt;
                }
                else
                {
                    break;
                }
            }

            ExecDX9(cmd1, cmd1_cnt, forceExecute);
            hr = cmd1[0].retval;

            if (SUCCEEDED(hr))
            {
                DX9Command cmd2[] =
                {
                  { DX9Command::QUERY_INTERFACE, { uint64_t(&tex9), uint64_t(&IID_IDirect3DBaseTexture9), uint64(&basetex9) } },
                  { DX9Command::SET_TEXTURE_AUTOGEN_FILTER_TYPE, { uint64_t(tex9), D3DTEXF_LINEAR } }
                };

                if (!auto_mip)
                    cmd2[1].func = DX9Command::NOP;

                ExecDX9(cmd2, countof(cmd2), forceExecute);

                if (desc.isRenderTarget || is_depthbuf)
                {
                    DX9Command cmd3 = { DX9Command::GET_TEXTURE_SURFACE_LEVEL, { uint64_t(&tex9), 0, uint64_t(&surf9) } };
                    ExecDX9(&cmd3, 1, forceExecute);
                }

                success = true;
            }
            else
            {
                Logger::Error("failed to create texture:\n%s\n", D3D9ErrorText(hr));
            }
        }
        break;

        case TEXTURE_TYPE_CUBE:
        {
            DVASSERT(cubetex9 == nullptr);

            uint32 cmd1_cnt = 1;
            DX9Command cmd1[128] =
            {
              { DX9Command::CREATE_CUBE_TEXTURE, { desc.width, mip_count, usage, static_cast<uint64>(DX9_TextureFormat(desc.format)), static_cast<uint64>(pool), uint64_t(&cubetex9), NULL } }
            };

            DVASSERT(desc.levelCount * 6 <= countof(desc.initialData));

            for (unsigned f = 0; f != countof(textureFaces); ++f)
            {
                for (unsigned m = 0; m != desc.levelCount; ++m)
                {
                    DX9Command* cmd = cmd1 + cmd1_cnt;
                    void* data = desc.initialData[f * desc.levelCount + m];

                    if (data)
                    {
                        Size2i sz = TextureExtents(Size2i(desc.width, desc.height), m);
                        uint32 data_sz = TextureSize(desc.format, sz.dx, sz.dy);
                        cmd->func = DX9Command::UPDATE_CUBETEXTURE_LEVEL;
                        cmd->arg[0] = uint64_t(&cubetex9);
                        cmd->arg[1] = m;
                        cmd->arg[2] = textureFaceToD3DFace[f];
                        cmd->arg[3] = (uint64)(data);
                        cmd->arg[4] = data_sz;
                        cmd->arg[5] = desc.format;
                        ++cmd1_cnt;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            ExecDX9(cmd1, cmd1_cnt, forceExecute);
            hr = cmd1[0].retval;

            if (SUCCEEDED(hr))
            {
                tex9 = nullptr;

                uint32 cmd2Count = 2;
                DX9Command cmd2[2 + 6] =
                {
                  { DX9Command::QUERY_INTERFACE, { uint64_t(&cubetex9), uint64_t(&IID_IDirect3DBaseTexture9), uint64(&basetex9) } },
                  { DX9Command::SET_TEXTURE_AUTOGEN_FILTER_TYPE, { uint64_t(cubetex9), D3DTEXF_LINEAR } }
                };

                if (!auto_mip)
                    cmd2[1].func = DX9Command::NOP;

                if (desc.isRenderTarget)
                {
                    for (DAVA::uint32 f = 0; f != 6; ++f)
                    {
                        cmd2[cmd2Count] = { DX9Command::GET_CUBE_SURFACE_LEVEL, { uint64(&cubetex9), uint64(textureFaceToD3DFace[f]), 0, uint64(&cubesurf9[f]) } };
                        ++cmd2Count;
                    }
                }

                ExecDX9(cmd2, cmd2Count, forceExecute);

                success = true;
            }
            else
            {
                Logger::Error("failed to create texture:\n%s\n", D3D9ErrorText(hr));
            }
        }
        break;
        default:
            break;
        }
    }

    return success;
}

//------------------------------------------------------------------------------

void TextureDX9_t::Destroy(bool forceExecute)
{
    DVASSERT(!isMapped);

    DX9Command cmd[6 + 6] =
    {
      { rt_surf9 ? DX9Command::RELEASE : DX9Command::NOP, { uint64_t(&rt_surf9) } },
      { rt_tex9 ? DX9Command::RELEASE : DX9Command::NOP, { uint64_t(&rt_tex9) } },
      { surf9 ? DX9Command::RELEASE : DX9Command::NOP, { uint64_t(&surf9) } },
      { cubetex9 ? DX9Command::RELEASE : DX9Command::NOP, { uint64_t(&cubetex9) } },
      { tex9 ? DX9Command::RELEASE : DX9Command::NOP, { uint64_t(&tex9) } },
      { basetex9 ? DX9Command::RELEASE : DX9Command::NOP, { uint64_t(&basetex9) } },
    };

    for (DAVA::uint32 i = 0; i < 6; ++i)
        cmd[6 + i] = { cubesurf9[i] ? DX9Command::RELEASE : DX9Command::NOP, { uint64(&cubesurf9[i]) } };

    bool cancelRecreate = true;
    for (size_t i = 0; i < countof(cmd); ++i)
    {
        if (cmd[i].func != DX9Command::NOP)
        {
            cancelRecreate = false;
            break;
        }
    }

    if (cancelRecreate)
    {
        SetRecreatePending(false);
    }

    ExecDX9(cmd, countof(cmd), forceExecute);
    surf9 = nullptr;
    tex9 = nullptr;
    cubetex9 = nullptr;
    basetex9 = nullptr;
    rt_surf9 = nullptr;
    rt_tex9 = nullptr;
    memset(cubesurf9, 0, sizeof(cubesurf9));

    if (!RecreatePending() && (mappedData != nullptr))
    {
        DVASSERT(!isMapped);
        ::free(mappedData);
        mappedData = nullptr;
    }

    MarkRestored();
}

//------------------------------------------------------------------------------

static Handle
dx9_Texture_Create(const Texture::Descriptor& desc)
{
    Handle handle = TextureDX9Pool::Alloc();
    TextureDX9_t* tex = TextureDX9Pool::Get(handle);

    if (tex->Create(desc) == false)
    {
        TextureDX9Pool::Free(handle);
        handle = InvalidHandle;
    }

    return handle;
}

//------------------------------------------------------------------------------

static void
dx9_Texture_Delete(Handle tex, bool forceExecute)
{
    TextureDX9_t* self = TextureDX9Pool::Get(tex);
    self->SetRecreatePending(false);
    self->Destroy(forceExecute);
    TextureDX9Pool::Free(tex);
}

//------------------------------------------------------------------------------

static void*
dx9_Texture_Map(Handle tex, unsigned level, TextureFace face)
{
    TextureDX9_t* self = TextureDX9Pool::Get(tex);

    TextureFormat format = self->CreationDesc().format;
    uint32 data_sz = TextureSize(format, self->CreationDesc().width, self->CreationDesc().height, level);
    self->mappedData = ::malloc(data_sz);

    void* mem = nullptr;

    D3DLOCKED_RECT rc = {};

    if (self->cubetex9)
    {
        DVASSERT(!self->CreationDesc().isRenderTarget);
        DVASSERT(face != TEXTURE_FACE_NONE);

        D3DCUBEMAP_FACES f = textureFaceToD3DFace[face];
        DX9Command cmd = { DX9Command::READ_CUBETEXTURE_LEVEL, { uint64_t(&self->cubetex9), level, static_cast<uint64>(face), data_sz, static_cast<uint64>(format), uint64(self->mappedData) } };
        ExecDX9(&cmd, 1, false);
        if (SUCCEEDED(cmd.retval))
        {
            self->mappedLevel = level;
            self->mappedFace = face;
            self->isMapped = true;
        }
    }
    else
    {
        bool dataAvailable = !self->CreationDesc().isRenderTarget;

        if (self->CreationDesc().isRenderTarget)
        {
            DVASSERT(level == 0);

            if (self->rt_tex9 == nullptr)
            {
                DX9Command cmd1 = { DX9Command::CREATE_TEXTURE, { self->CreationDesc().width, self->CreationDesc().height, 1, 0, static_cast<uint64>(DX9_TextureFormat(format)), D3DPOOL_SYSTEMMEM, uint64_t(&self->rt_tex9), 0 } };

                ExecDX9(&cmd1, 1, false);

                if (SUCCEEDED(cmd1.retval))
                {
                    DX9Command cmd2 = { DX9Command::GET_TEXTURE_SURFACE_LEVEL, { uint64_t(&self->rt_tex9), 0, uint64_t(&self->rt_surf9) } };
                    ExecDX9(&cmd2, 1, false);
                }
            }

            if (self->rt_tex9 && self->rt_surf9)
            {
                DX9Command cmd3 = { DX9Command::GET_RENDERTARGET_DATA, { uint64_t(&self->surf9), uint64_t(&self->rt_surf9) } };
                ExecDX9(&cmd3, 1, false);
                dataAvailable = (cmd3.retval == D3D_OK);
            }
        }

        if (dataAvailable)
        {
            IDirect3DTexture9** tex = (self->CreationDesc().isRenderTarget) ? &self->rt_tex9 : &self->tex9;
            DX9Command cmd = { DX9Command::READ_TEXTURE_LEVEL, { uint64_t(tex), level, data_sz, static_cast<uint64>(format), uint64(self->mappedData) } };
            ExecDX9(&cmd, 1, false);
            if (SUCCEEDED(cmd.retval))
            {
                self->mappedLevel = level;
                self->isMapped = true;
            }
        }
    }

    return self->mappedData;
}

//------------------------------------------------------------------------------

static void
dx9_Texture_Unmap(Handle tex)
{
    TextureDX9_t* self = TextureDX9Pool::Get(tex);

    DVASSERT(self->isMapped);

    Size2i sz = TextureExtents(Size2i(self->CreationDesc().width, self->CreationDesc().height), self->mappedLevel);
    uint64 data_sz = TextureSize(self->CreationDesc().format, sz.dx, sz.dy);

    HRESULT hr;

    if (self->cubetex9)
    {
        D3DCUBEMAP_FACES f = textureFaceToD3DFace[self->mappedFace];
        DX9Command cmd = { DX9Command::UPDATE_CUBETEXTURE_LEVEL, { uint64_t(&self->cubetex9), self->mappedLevel, static_cast<uint64>(f), uint64(self->mappedData), data_sz, static_cast<uint64>(self->CreationDesc().format) } };
        ExecDX9(&cmd, 1, false);
        hr = cmd.retval;
    }
    else
    {
        IDirect3DTexture9** tex = (self->CreationDesc().isRenderTarget) ? &self->rt_tex9 : &self->tex9;
        DX9Command cmd = { DX9Command::UPDATE_TEXTURE_LEVEL, { uint64_t(tex), self->mappedLevel, uint64(self->mappedData), data_sz, static_cast<uint64>(self->CreationDesc().format) } };
        ExecDX9(&cmd, 1, false);
        hr = cmd.retval;
    }

    if (FAILED(hr))
    {
        Logger::Error("Failed to update texture (0x%08X) : %s", hr, D3D9ErrorText(hr));
    }

    self->isMapped = false;
    self->MarkRestored();
}

//------------------------------------------------------------------------------

#define DX9_UPDATE_TEXTURE_USING_MAP 0

static void
dx9_Texture_Update(Handle tex, const void* data, uint32 level, TextureFace face)
{
    TextureDX9_t* self = TextureDX9Pool::Get(tex);

#if (DX9_UPDATE_TEXTURE_USING_MAP)
    void* dst = dx9_Texture_Map(tex, level, face);
    uint32 sz = TextureSize(self->format, self->width, self->height, level);
    memcpy(dst, data, sz);
    dx9_Texture_Unmap(tex);
#else
    Size2i sz = TextureExtents(Size2i(self->CreationDesc().width, self->CreationDesc().height), level);
    uint64 data_sz = TextureSize(self->CreationDesc().format, sz.dx, sz.dy);

    HRESULT hr(0);

    if (self->cubetex9)
    {
        DVASSERT(face != TEXTURE_FACE_NONE);
        D3DCUBEMAP_FACES f = textureFaceToD3DFace[face];
        DX9Command cmd = { DX9Command::UPDATE_CUBETEXTURE_LEVEL, { uint64_t(&self->cubetex9), level, static_cast<uint64>(f), uint64(data), data_sz, static_cast<uint64>(self->CreationDesc().format) } };
        ExecDX9(&cmd, 1, false);
        hr = cmd.retval;
    }
    else
    {
        IDirect3DTexture9** tex = (self->CreationDesc().isRenderTarget) ? &self->rt_tex9 : &self->tex9;
        DX9Command cmd = { DX9Command::UPDATE_TEXTURE_LEVEL, { uint64_t(tex), level, uint64(data), data_sz, static_cast<uint64>(self->CreationDesc().format) } };
        ExecDX9(&cmd, 1, false);
        hr = cmd.retval;
    }

    if (hr)
    {
        Logger::Error("Failed to update texture (0x%08X) : %s", hr, D3D9ErrorText(hr));
    }
#endif

    self->MarkRestored();
}

//------------------------------------------------------------------------------

static bool dx9_Texture_NeedRestore(Handle tex)
{
    TextureDX9_t* self = TextureDX9Pool::Get(tex);
    return self->NeedRestore();
}

//==============================================================================

namespace TextureDX9
{
void Init(uint32 maxCount)
{
    TextureDX9Pool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_Texture_Create = &dx9_Texture_Create;
    dispatch->impl_Texture_Delete = &dx9_Texture_Delete;
    dispatch->impl_Texture_Map = &dx9_Texture_Map;
    dispatch->impl_Texture_Unmap = &dx9_Texture_Unmap;
    dispatch->impl_Texture_Update = &dx9_Texture_Update;
    dispatch->impl_Texture_NeedRestore = &dx9_Texture_NeedRestore;
}

void SetToRHI(Handle tex, unsigned unit_i)
{
    TextureDX9_t* self = TextureDX9Pool::Get(tex);

    _D3D9_Device->SetTexture(unit_i, self->basetex9);
    self->lastUnit = unit_i;
}

void SetAsRenderTarget(Handle tex, unsigned target_i, TextureFace face)
{
    TextureDX9_t* self = TextureDX9Pool::Get(tex);

    if (self->lastUnit != DAVA::InvalidIndex)
    {
        _D3D9_Device->SetTexture(self->lastUnit, NULL);
        self->lastUnit = DAVA::InvalidIndex;
    }

    if (self->cubetex9)
    {
        DVASSERT(face != TEXTURE_FACE_NONE);
        DVASSERT(self->cubesurf9[face] != nullptr);
        DX9_CALL(_D3D9_Device->SetRenderTarget(target_i, self->cubesurf9[face]), "SetRenderTarget");
    }
    else
    {
        DVASSERT(face == TEXTURE_FACE_NONE);
        DVASSERT(self->surf9 != nullptr);
        DX9_CALL(_D3D9_Device->SetRenderTarget(target_i, self->surf9), "SetRenderTarget");
    }
}

void SetAsDepthStencil(Handle tex)
{
    TextureDX9_t* self = TextureDX9Pool::Get(tex);

    DX9_CALL(_D3D9_Device->SetDepthStencilSurface(self->surf9), "SetDepthStencilSurface");
}

void ReleaseAll()
{
    TextureDX9Pool::ReleaseAll();
}

void ReCreateAll()
{
    LCP;
    TextureDX9Pool::ReCreateAll();
}

void LogUnrestoredBacktraces()
{
    TextureDX9Pool::LogUnrestoredBacktraces();
}

unsigned NeedRestoreCount()
{
    return TextureDX9Pool::PendingRestoreCount();
}

void ResolveMultisampling(Handle from, Handle to)
{
    TextureDX9_t* fromTexture = TextureDX9Pool::Get(from);
    DVASSERT(fromTexture != nullptr);
    DVASSERT(fromTexture->surf9 != nullptr);

    TextureDX9_t* toTexture = (to == InvalidHandle) ? nullptr : TextureDX9Pool::Get(to);
    IDirect3DSurface9* targetSurface = (toTexture == nullptr) ? _D3D9_BackBuf : toTexture->surf9;
    DVASSERT(targetSurface != nullptr);

    _D3D9_Device->StretchRect(fromTexture->surf9, nullptr, targetSurface, nullptr, D3DTEXF_POINT);
}
}

} // namespace rhi
