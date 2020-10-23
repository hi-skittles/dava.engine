#include "rhi_Utils.h"
#include "../rhi_Public.h"

#include "Concurrency/Spinlock.h"

void Trace(const char* format, ...)
{
#if 0
    static DAVA::Spinlock _TraceSync;
    static char _TraceBuf[4096];

    _TraceSync.Lock();

    va_list  arglist;

    va_start(arglist, format);
#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)
    _vsnprintf(_TraceBuf, countof(_TraceBuf), format, arglist);
#else
    vsnprintf(_TraceBuf, countof(_TraceBuf), format, arglist);
#endif
    va_end(arglist);

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)
    ::OutputDebugStringA(_TraceBuf);
#else
    puts(_TraceBuf);
#endif

    _TraceSync.Unlock();
#endif
}

namespace rhi
{
uint32 TextureStride(TextureFormat format, Size2i size, uint32 level)
{
    uint32 width = TextureExtents(size, level).dx;

    switch (format)
    {
    case TEXTURE_FORMAT_R8G8B8A8:
        return width * sizeof(uint32);
    case TEXTURE_FORMAT_R8G8B8:
        return width * 3 * sizeof(uint8);
    case TEXTURE_FORMAT_R4G4B4A4:
    case TEXTURE_FORMAT_R5G5B5A1:
    case TEXTURE_FORMAT_R5G6B5:
    case TEXTURE_FORMAT_R16:
    case TEXTURE_FORMAT_D16:
        return width * sizeof(uint16);
    case TEXTURE_FORMAT_R8:
        return width * sizeof(uint8);
    case TEXTURE_FORMAT_D24S8:
        return width * sizeof(uint32);

    case TEXTURE_FORMAT_R32F:
        return width * sizeof(float32);

    case TEXTURE_FORMAT_DXT1:
        return 8 * std::max(1u, (width + 3) / 4);
    case TEXTURE_FORMAT_DXT3:
    case TEXTURE_FORMAT_DXT5:
        return 16 * std::max(1u, (width + 3) / 4);
    default:
        return 0;
    }
}

//------------------------------------------------------------------------------

Size2i TextureExtents(Size2i size, uint32 level)
{
    return Size2i(std::max(1, size.dx >> level), std::max(1, size.dy >> level));
}

//------------------------------------------------------------------------------

uint32 TextureSize(TextureFormat format, uint32 width, uint32 height, uint32 level)
{
    Size2i ext = TextureExtents(Size2i(width, height), level);
    uint32 sz = 0;

    switch (format)
    {
    case TEXTURE_FORMAT_R8G8B8A8:
    case TEXTURE_FORMAT_R8G8B8X8:
        sz = ext.dx * ext.dy * sizeof(uint32);
        break;

    case TEXTURE_FORMAT_R8G8B8:
        sz = ext.dx * ext.dy * 3 * sizeof(uint8);
        break;

    case TEXTURE_FORMAT_R5G5B5A1:
    case TEXTURE_FORMAT_R5G6B5:
        sz = ext.dx * ext.dy * sizeof(uint16);
        break;

    case TEXTURE_FORMAT_R4G4B4A4:
        sz = ext.dx * ext.dy * sizeof(uint16);
        break;

    case TEXTURE_FORMAT_A16R16G16B16:
        sz = ext.dx * ext.dy * sizeof(uint16);
        break;

    case TEXTURE_FORMAT_A32R32G32B32:
        sz = ext.dx * ext.dy * sizeof(float32);
        break;

    case TEXTURE_FORMAT_R8:
        sz = ext.dx * ext.dy * sizeof(uint8);
        break;

    case TEXTURE_FORMAT_R16:
        sz = ext.dx * ext.dy * sizeof(uint16);
        break;

    case TEXTURE_FORMAT_DXT1:
    {
        int ww = ext.dx >> 2;
        int hh = ext.dy >> 2;

        if (!ww)
            ww = 1;
        if (!hh)
            hh = 1;

        sz = (ww * hh) << 3;
    }
    break;

    case TEXTURE_FORMAT_DXT3:
    case TEXTURE_FORMAT_DXT5:
    {
        int ww = ext.dx >> 2;
        int hh = ext.dy >> 2;

        if (!ww)
            ww = 1;
        if (!hh)
            hh = 1;

        sz = (ww * hh) << 4;
    }
    break;

    case TEXTURE_FORMAT_PVRTC_4BPP_RGBA:
    {
        uint32 block_h = 8;
        uint32 block_w = 8;

        sz = ((height + block_h - 1) / block_h) * ((width + block_w - 1) / block_w) * (sizeof(uint64) * 4);
    }
    break;

    case TEXTURE_FORMAT_PVRTC_2BPP_RGBA:
    {
        uint32 block_h = 16;
        uint32 block_w = 8;

        sz = ((height + block_h - 1) / block_h) * ((width + block_w - 1) / block_w) * (sizeof(uint64) * 4);
    }
    break;

    case TEXTURE_FORMAT_PVRTC2_4BPP_RGB:
    case TEXTURE_FORMAT_PVRTC2_4BPP_RGBA:
    {
        uint32 block_h = 4;
        uint32 block_w = 4;

        sz = ((height + block_h - 1) / block_h) * ((width + block_w - 1) / block_w) * sizeof(uint64);
    }
    break;

    case TEXTURE_FORMAT_PVRTC2_2BPP_RGB:
    case TEXTURE_FORMAT_PVRTC2_2BPP_RGBA:
    {
        uint32 block_h = 4;
        uint32 block_w = 8;

        sz = ((height + block_h - 1) / block_h) * ((width + block_w - 1) / block_w) * sizeof(uint64);
    }
    break;

    case TEXTURE_FORMAT_ATC_RGB:
        sz = ((ext.dx + 3) / 4) * ((ext.dy + 3) / 4) * 8;
        break;

    case TEXTURE_FORMAT_ATC_RGBA_EXPLICIT:
    case TEXTURE_FORMAT_ATC_RGBA_INTERPOLATED:
        sz = ((ext.dx + 3) / 4) * ((ext.dy + 3) / 4) * 16;
        break;

    case TEXTURE_FORMAT_ETC1:
    case TEXTURE_FORMAT_ETC2_R8G8B8:
    {
        int ww = ext.dx >> 2;
        int hh = ext.dy >> 2;

        if (!ww)
            ww = 1;
        if (!hh)
            hh = 1;

        sz = (ww * hh) << 3;
    }
    break;

    case TEXTURE_FORMAT_ETC2_R8G8B8A8:
    case TEXTURE_FORMAT_ETC2_R8G8B8A1:
    {
        int ww = ext.dx >> 2;
        int hh = ext.dy >> 2;

        if (!ww)
            ww = 1;
        if (!hh)
            hh = 1;

        sz = (ww * hh) << 4;
    }
    break;

    case TEXTURE_FORMAT_EAC_R11_UNSIGNED:
    case TEXTURE_FORMAT_EAC_R11_SIGNED:
    {
        int ww = ext.dx >> 2;
        int hh = ext.dy >> 2;

        if (!ww)
            ww = 1;
        if (!hh)
            hh = 1;

        sz = (ww * hh) << 3;
    }
    break;

    case TEXTURE_FORMAT_EAC_R11G11_UNSIGNED:
    case TEXTURE_FORMAT_EAC_R11G11_SIGNED:
    {
        int ww = ext.dx >> 2;
        int hh = ext.dy >> 2;

        if (!ww)
            ww = 1;
        if (!hh)
            hh = 1;

        sz = (ww * hh) << 4;
    }
    break;

    case TEXTURE_FORMAT_D16:
        sz = ext.dx * ext.dy * sizeof(uint16);
        break;

    case TEXTURE_FORMAT_D24S8:
        sz = ext.dx * ext.dy * sizeof(uint32);
        break;

    case TEXTURE_FORMAT_R32F:
        sz = ext.dx * ext.dy * sizeof(float32);
        break;

    case TEXTURE_FORMAT_RG32F:
        sz = ext.dx * ext.dy * sizeof(float32) * 2;
        break;

    case TEXTURE_FORMAT_RGBA32F:
        sz = ext.dx * ext.dy * sizeof(float32) * 4;
        break;

    default:
        break;
    }

    return sz;
}

uint32 NativeColorRGBA(float red, float green, float blue, float alpha)
{
    uint32 color = 0;
    int32 r = int32(red * 255.0f);
    int32 g = int32(green * 255.0f);
    int32 b = int32(blue * 255.0f);
    int32 a = int32(alpha * 255.0f);

    DVASSERT((r >= 0) && (r <= 0xff) && (g >= 0) && (g <= 0xff) && (b >= 0) && (b <= 0xff) && (a >= 0) && (a <= 0xff));

    switch (HostApi())
    {
    case RHI_DX9:
        color = static_cast<uint32>(((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF));
        break;

    case RHI_DX11:
    case RHI_GLES2:
    case RHI_METAL:
    case RHI_NULL_RENDERER:
        color = static_cast<uint32>(((a & 0xFF) << 24) | ((b & 0xFF) << 16) | ((g & 0xFF) << 8) | (r & 0xFF));
        break;

    default:
        DVASSERT(!"kaboom!"); // to shut up goddamn warning
        break;
    }

    return color;
}

uint32 NativeColorRGBA(uint32 color)
{
    uint32 c = 0;

    switch (HostApi())
    {
    case RHI_DX9:
        c = (color & 0xff000000) | ((color & 0x000000ff) << 16) | (color & 0x0000ff00) | ((color & 0x00ff0000) >> 16);
        break;

    case RHI_DX11:
    case RHI_GLES2:
    case RHI_METAL:
    case RHI_NULL_RENDERER:
        c = color;
        break;

    default:
        DVASSERT(!"kaboom!"); // to shut up goddamn warning
        break;
    }

    return c;
}

bool NeedInvertProjection(const RenderPassConfig& passDesc)
{
    bool isRT = (passDesc.colorBuffer[0].texture != rhi::InvalidHandle) ||
    (passDesc.colorBuffer[1].texture != rhi::InvalidHandle) ||
    (passDesc.depthStencilBuffer.texture != rhi::InvalidHandle && passDesc.depthStencilBuffer.texture != rhi::DefaultDepthBuffer);

    bool isCubemapRT = (passDesc.colorBuffer[0].textureFace != TEXTURE_FACE_NONE) || (passDesc.colorBuffer[1].textureFace != TEXTURE_FACE_NONE);

    return isRT && ((!DeviceCaps().isUpperLeftRTOrigin && !isCubemapRT) || (isCubemapRT && DeviceCaps().isUpperLeftRTOrigin));
}
}
