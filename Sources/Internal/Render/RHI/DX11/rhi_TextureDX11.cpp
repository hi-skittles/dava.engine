#include "rhi_DX11.h"
#include "../Common/rhi_FormatConversion.h"

namespace rhi
{
struct TextureDX11_t
{
    Texture::Descriptor descriptor;
    uint32 arraySize = 1;
    uint32 mipLevelCount = 0;
    uint32 lastUnit = DAVA::InvalidIndex;
    uint32 mappedLevel = 0;
    TextureFace mappedFace = TEXTURE_FACE_NONE;
    ID3D11Texture2D* tex2d = nullptr;
    ID3D11ShaderResourceView* tex2d_srv = nullptr;
    ID3D11DepthStencilView* tex2d_dsv = nullptr;
    ID3D11Texture2D* tex2d_copy = nullptr;
    void* mappedData = nullptr;
    bool isMapped = false;

    struct RTView
    {
        ID3D11RenderTargetView* view = nullptr;
        uint32 level = 0;
        TextureFace face = TEXTURE_FACE_NONE;
        RTView(ID3D11RenderTargetView* v, uint32 l, TextureFace f);
    };
    std::vector<RTView> rt_view;
    ID3D11RenderTargetView* getRenderTargetView(uint32 level, TextureFace face);
};
using TextureDX11Pool = ResourcePool<TextureDX11_t, RESOURCE_TEXTURE, Texture::Descriptor, true>;
RHI_IMPL_POOL(TextureDX11_t, RESOURCE_TEXTURE, Texture::Descriptor, true);

TextureDX11_t::RTView::RTView(ID3D11RenderTargetView* v, uint32 l, TextureFace f)
    : view(v)
    , level(l)
    , face(f)
{
}

ID3D11RenderTargetView* TextureDX11_t::getRenderTargetView(uint32 level, TextureFace face)
{
    for (const RTView& v : rt_view)
    {
        if (v.level == level && v.face == face)
            return v.view;
    }

    D3D11_RENDER_TARGET_VIEW_DESC desc = {};
    desc.Format = DX11_TextureFormat(descriptor.format);

    if (arraySize == 6)
    {
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        desc.Texture2DArray.MipSlice = level;
        desc.Texture2DArray.ArraySize = 1;

        switch (face)
        {
        case TEXTURE_FACE_POSITIVE_X:
            desc.Texture2DArray.FirstArraySlice = 0;
            break;
        case TEXTURE_FACE_NEGATIVE_X:
            desc.Texture2DArray.FirstArraySlice = 1;
            break;
        case TEXTURE_FACE_POSITIVE_Y:
            desc.Texture2DArray.FirstArraySlice = 2;
            break;
        case TEXTURE_FACE_NEGATIVE_Y:
            desc.Texture2DArray.FirstArraySlice = 3;
            break;
        case TEXTURE_FACE_POSITIVE_Z:
            desc.Texture2DArray.FirstArraySlice = 4;
            break;
        case TEXTURE_FACE_NEGATIVE_Z:
            desc.Texture2DArray.FirstArraySlice = 5;
            break;
        default:
            break;
        }
    }
    else
    {
        desc.ViewDimension = (descriptor.sampleCount > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
        if (descriptor.sampleCount == 1)
            desc.Texture2D.MipSlice = level;
    }

    ID3D11RenderTargetView* rtv = nullptr;
    if (DX11DeviceCommand(DX11Command::CREATE_RENDER_TARGET_VIEW, tex2d, &desc, &rtv))
    {
        rt_view.emplace_back(rtv, level, face);
    }

    return rtv;
}

static Handle dx11_Texture_Create(const Texture::Descriptor& desc)
{
    DVASSERT(desc.levelCount);

    bool need_srv = true;
    bool need_dsv = false;
    bool need_copy = false;

    D3D11_TEXTURE2D_DESC desc2d = {};
    desc2d.Width = desc.width;
    desc2d.Height = desc.height;
    desc2d.MipLevels = desc.levelCount;
    desc2d.ArraySize = 1;
    desc2d.Format = DX11_TextureFormat(desc.format);
    desc2d.SampleDesc.Count = desc.sampleCount;
    desc2d.SampleDesc.Quality = 0;
    desc2d.Usage = D3D11_USAGE_DEFAULT;
    desc2d.BindFlags = (desc.sampleCount > 1) ? D3D11_BIND_RENDER_TARGET : D3D11_BIND_SHADER_RESOURCE;

    DVASSERT(desc2d.Format != DXGI_FORMAT_UNKNOWN);

    if (desc.type == TEXTURE_TYPE_CUBE)
    {
        DVASSERT(desc.sampleCount == 1);
        desc2d.ArraySize = 6;
        desc2d.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
    }

    if (desc.autoGenMipmaps)
    {
        desc2d.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }

    if (desc.isRenderTarget)
    {
        need_srv = (desc.sampleCount == 1);
        desc2d.BindFlags |= D3D11_BIND_RENDER_TARGET;
        desc2d.MipLevels = 1;
    }

    if (desc.cpuAccessRead)
    {
        DVASSERT(desc.type == TEXTURE_TYPE_2D);
        DVASSERT(!desc.cpuAccessWrite);
        need_copy = true;
    }

    if (desc.format == TEXTURE_FORMAT_D16 || desc.format == TEXTURE_FORMAT_D24S8)
    {
        desc2d.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        need_srv = false;
        need_dsv = true;
    }

    bool useInitialData = false;
    D3D11_SUBRESOURCE_DATA data[128] = {};
    DVASSERT(countof(data) <= countof(desc.initialData));

    for (uint32 s = 0; s != desc2d.ArraySize; ++s)
    {
        for (uint32 m = 0; m != desc.levelCount; ++m)
        {
            uint32 di = s * desc.levelCount + m;
            if (desc.initialData[di])
            {
                // multisampled texture should be created without initial data
                DVASSERT(desc2d.SampleDesc.Count == 1);

                data[di].pSysMem = desc.initialData[di];
                data[di].SysMemPitch = TextureStride(desc.format, Size2i(desc.width, desc.height), m);

                if (desc.format == TEXTURE_FORMAT_R8G8B8A8)
                {
                    _SwapRB8(desc.initialData[m], desc.initialData[m], TextureSize(desc.format, desc.width, desc.height, m));
                }
                else if (desc.format == TEXTURE_FORMAT_R4G4B4A4)
                {
                    _SwapRB4(desc.initialData[m], desc.initialData[m], TextureSize(desc.format, desc.width, desc.height, m));
                }
                else if (desc.format == TEXTURE_FORMAT_R5G5B5A1)
                {
                    _SwapRB5551(desc.initialData[m], desc.initialData[m], TextureSize(desc.format, desc.width, desc.height, m));
                }

                useInitialData = true;
            }
            else
            {
                break;
            }
        }
    }

    Handle handle = InvalidHandle;
    ID3D11Texture2D* tex2d = nullptr;
    if (DX11DeviceCommand(DX11Command::CREATE_TEXTURE_2D, &desc2d, (useInitialData) ? data : nullptr, &tex2d))
    {
        handle = TextureDX11Pool::Alloc();
        TextureDX11_t* tex = TextureDX11Pool::Get(handle);

        tex->tex2d = tex2d;
        tex->descriptor = desc;
        tex->arraySize = desc2d.ArraySize;
        tex->mipLevelCount = desc2d.MipLevels;
        tex->mappedData = nullptr;
        tex->isMapped = false;

        memset(tex->descriptor.initialData, 0, sizeof(tex->descriptor.initialData));

        if (need_srv)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

            srv_desc.Format = desc2d.Format;

            if (desc.type == TEXTURE_TYPE_CUBE)
            {
                DVASSERT(desc.sampleCount == 1);
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                srv_desc.TextureCube.MipLevels = desc2d.MipLevels;
            }
            else if (desc.sampleCount > 1)
            {
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srv_desc.Texture2D.MipLevels = desc2d.MipLevels;
            }

            ID3D11ShaderResourceView* srv = nullptr;
            if (DX11DeviceCommand(DX11Command::CREATE_SHADER_RESOURCE_VIEW, tex2d, &srv_desc, &srv))
            {
                tex->tex2d_srv = srv;
            }
        }

        if (need_copy)
        {
            ID3D11Texture2D* copy = nullptr;
            desc2d.Usage = D3D11_USAGE_STAGING;
            desc2d.BindFlags = 0;
            desc2d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            if (DX11DeviceCommand(DX11Command::CREATE_TEXTURE_2D, &desc2d, NULL, &copy))
            {
                tex->tex2d_copy = copy;
            }
        }

        if (need_dsv)
        {
            DVASSERT(desc.type == TEXTURE_TYPE_2D);

            D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
            dsv_desc.Format = desc2d.Format;
            dsv_desc.ViewDimension = (desc.sampleCount > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;

            ID3D11DepthStencilView* dsv = nullptr;
            if (DX11DeviceCommand(DX11Command::CREATE_DEPTH_STENCIL_VIEW, tex2d, &dsv_desc, &dsv))
            {
                tex->tex2d_dsv = dsv;
            }
        }
    }

    return handle;
}

static void dx11_Texture_Delete(Handle tex, bool)
{
    TextureDX11_t* self = TextureDX11Pool::Get(tex);

    for (const TextureDX11_t::RTView& v : self->rt_view)
        v.view->Release();

    if (self->mappedData)
        free(self->mappedData);

    DAVA::SafeRelease(self->tex2d_srv);
    DAVA::SafeRelease(self->tex2d_dsv);
    DAVA::SafeRelease(self->tex2d);
    DAVA::SafeRelease(self->tex2d_copy);
    self->tex2d_srv = nullptr;
    self->tex2d_dsv = nullptr;
    self->tex2d = nullptr;
    self->tex2d_copy = nullptr;
    self->mappedData = nullptr;
    self->rt_view.clear();

    TextureDX11Pool::Free(tex);
}

static void* dx11_Texture_Map(Handle tex, uint32 level, TextureFace face)
{
    TextureDX11_t* self = TextureDX11Pool::Get(tex);

    DVASSERT(!self->isMapped);

    TextureFormat fmt = self->descriptor.format;
    uint32 w = self->descriptor.width;
    uint32 h = self->descriptor.height;

    if (self->descriptor.cpuAccessRead)
    {
        DVASSERT(self->tex2d_copy);

        D3D11_MAPPED_SUBRESOURCE res = {};
        DX11Command cmd[] =
        {
          { DX11Command::COPY_RESOURCE, self->tex2d_copy, self->tex2d },
          { DX11Command::MAP, self->tex2d_copy, 0, D3D11_MAP_READ, 0, &res }
        };
        ExecDX11(cmd, countof(cmd));
        DX11Check(cmd[1].retval);

        self->mappedData = res.pData;
        self->mappedLevel = level;
        self->isMapped = true;
    }
    else
    {
        self->mappedData = ::realloc(self->mappedData, TextureSize(fmt, w, h, level));
        self->mappedLevel = level;
        self->mappedFace = face;
        self->isMapped = true;
    }

    if (fmt == TEXTURE_FORMAT_R8G8B8A8)
    {
        _SwapRB8(self->mappedData, self->mappedData, TextureSize(fmt, w, h, self->mappedLevel));
    }
    else if (fmt == TEXTURE_FORMAT_R4G4B4A4)
    {
        _SwapRB4(self->mappedData, self->mappedData, TextureSize(fmt, w, h, self->mappedLevel));
    }
    else if (fmt == TEXTURE_FORMAT_R5G5B5A1)
    {
        _SwapRB5551(self->mappedData, self->mappedData, TextureSize(fmt, w, h, self->mappedLevel));
    }

    return self->mappedData;
}

static void dx11_Texture_Unmap(Handle tex)
{
    TextureDX11_t* self = TextureDX11Pool::Get(tex);

    DVASSERT(self->isMapped);

    TextureFormat fmt = self->descriptor.format;
    uint32 w = self->descriptor.width;
    uint32 h = self->descriptor.height;

    if (self->descriptor.cpuAccessRead)
    {
        DVASSERT(self->tex2d_copy);

        D3D11_MAPPED_SUBRESOURCE res = {};
        DX11Command cmd(DX11Command::UNMAP, self->tex2d_copy, 0);
        ExecDX11(&cmd, 1);

        self->isMapped = false;
        self->mappedData = nullptr;
    }
    else
    {
        if (fmt == TEXTURE_FORMAT_R8G8B8A8)
        {
            _SwapRB8(self->mappedData, self->mappedData, TextureSize(fmt, w, h, self->mappedLevel));
        }
        else if (fmt == TEXTURE_FORMAT_R4G4B4A4)
        {
            _SwapRB4(self->mappedData, self->mappedData, TextureSize(fmt, w, h, self->mappedLevel));
        }
        else if (fmt == TEXTURE_FORMAT_R5G5B5A1)
        {
            _SwapRB5551(self->mappedData, self->mappedData, TextureSize(fmt, w, h, self->mappedLevel));
        }

        uint32 rc_i = 0;
        uint32 face = 0;

        if (self->arraySize == 6)
        {
            switch (self->mappedFace)
            {
            case TEXTURE_FACE_POSITIVE_X:
                face = 0;
                break;
            case TEXTURE_FACE_NEGATIVE_X:
                face = 1;
                break;
            case TEXTURE_FACE_POSITIVE_Y:
                face = 2;
                break;
            case TEXTURE_FACE_NEGATIVE_Y:
                face = 3;
                break;
            case TEXTURE_FACE_POSITIVE_Z:
                face = 4;
                break;
            case TEXTURE_FACE_NEGATIVE_Z:
                face = 5;
                break;
            default:
                DVASSERT(0, "Invalid TextureFace provided");
            }

            rc_i = self->mappedLevel + (face * self->mipLevelCount);
        }
        else
        {
            rc_i = self->mappedLevel;
        }

        DX11Command cmd(DX11Command::UPDATE_SUBRESOURCE, self->tex2d, rc_i, NULL, self->mappedData, TextureStride(fmt, Size2i(w, h), self->mappedLevel), 0);
        ExecDX11(&cmd, 1);
        self->isMapped = false;

        ::free(self->mappedData);
        self->mappedData = nullptr;
    }
}

void dx11_Texture_Update(Handle tex, const void* data, uint32 level, TextureFace face)
{
    TextureDX11_t* self = TextureDX11Pool::Get(tex);
    void* dst = dx11_Texture_Map(tex, level, face);
    uint32 sz = TextureSize(self->descriptor.format, self->descriptor.width, self->descriptor.height, level);

    memcpy(dst, data, sz);
    dx11_Texture_Unmap(tex);
}

bool dx11_Texture_NeedRestore(Handle tex)
{
    return false;
}

namespace TextureDX11
{
void Init(uint32 maxCount)
{
    TextureDX11Pool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_Texture_Create = &dx11_Texture_Create;
    dispatch->impl_Texture_Delete = &dx11_Texture_Delete;
    dispatch->impl_Texture_Map = &dx11_Texture_Map;
    dispatch->impl_Texture_Unmap = &dx11_Texture_Unmap;
    dispatch->impl_Texture_Update = &dx11_Texture_Update;
    dispatch->impl_Texture_NeedRestore = &dx11_Texture_NeedRestore;
}

void SetToRHIFragment(Handle tex, uint32 unit_i, ID3D11DeviceContext* context)
{
    TextureDX11_t* self = TextureDX11Pool::Get(tex);
    DVASSERT(self->tex2d_srv != nullptr);
    context->PSSetShaderResources(unit_i, 1, &(self->tex2d_srv));
    self->lastUnit = unit_i;
}

void SetToRHIVertex(Handle tex, uint32 unit_i, ID3D11DeviceContext* context)
{
    TextureDX11_t* self = TextureDX11Pool::Get(tex);
    DVASSERT(self->tex2d_srv != nullptr);
    context->VSSetShaderResources(unit_i, 1, &(self->tex2d_srv));
}

void SetRenderTarget(Handle tex, uint32 level, TextureFace face, ID3D11DeviceContext* context, ID3D11RenderTargetView** view)
{
    TextureDX11_t* self = TextureDX11Pool::Get(tex);

    if (self->lastUnit != DAVA::InvalidIndex)
    {
        ID3D11ShaderResourceView* srv[1] = {};
        context->PSSetShaderResources(self->lastUnit, 1, srv);
        self->lastUnit = DAVA::InvalidIndex;
    }

    *view = self->getRenderTargetView(level, face);
}

void SetDepthStencil(Handle tex, ID3D11DepthStencilView** view)
{
    TextureDX11_t* self = TextureDX11Pool::Get(tex);

    *view = self->tex2d_dsv;
}

Size2i Size(Handle tex)
{
    TextureDX11_t* self = TextureDX11Pool::Get(tex);
    return Size2i(self->descriptor.width, self->descriptor.height);
}

void ResolveMultisampling(Handle from, Handle to, ID3D11DeviceContext* context)
{
    TextureDX11_t* fromTexture = TextureDX11Pool::Get(from);
    DVASSERT(fromTexture != nullptr);

    ID3D11Resource* fromResource = fromTexture->tex2d;
    DXGI_FORMAT fromFormat = DX11_TextureFormat(fromTexture->descriptor.format);

    ID3D11Resource* toResource = nullptr;
    if (to == InvalidHandle)
    {
        dx11.renderTargetView->GetResource(&toResource);

        D3D11_RENDER_TARGET_VIEW_DESC desc = {};
        dx11.renderTargetView->GetDesc(&desc);
        DVASSERT(desc.Format = fromFormat);
    }
    else
    {
        TextureDX11_t* toTexture = TextureDX11Pool::Get(to);
        toTexture->tex2d_srv->GetResource(&toResource);

        DVASSERT(fromFormat == DX11_TextureFormat(toTexture->descriptor.format));
    }
    DVASSERT(toResource != nullptr);

    UINT toIndex = D3D11CalcSubresource(0, 0, 1);
    UINT fromIndex = D3D11CalcSubresource(0, 0, 1);
    context->ResolveSubresource(toResource, toIndex, fromResource, fromIndex, fromFormat);

    toResource->Release();
}
}

} // namespace rhi
