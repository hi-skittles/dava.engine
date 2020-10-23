#ifndef __RHI_FORMAT_CONVERT_H__
#define __RHI_FORMAT_CONVERT_H__

namespace rhi
{
inline void
_FlipRGBA4_ABGR4(void* srcPtr, void* dstPtr, uint32 size)
{
    uint8* dst = static_cast<uint8*>(dstPtr);
    uint8* src = static_cast<uint8*>(srcPtr);
    uint8* srcEnd = src + size;
    while (src < srcEnd)
    {
        uint8 t0 = src[0];
        uint8 t1 = src[1];

        t0 = ((t0 & 0x0F) << 4) | ((t0 & 0xF0) >> 4);
        t1 = ((t1 & 0x0F) << 4) | ((t1 & 0xF0) >> 4);

        dst[0] = t1;
        dst[1] = t0;

        dst += 2;
        src += 2;
    }
}

//------------------------------------------------------------------------------

inline void
_ABGR1555toRGBA5551(void* srcPtr, void* dstPtr, uint32 size)
{
    uint16* dst = static_cast<uint16*>(dstPtr);
    uint16* src = static_cast<uint16*>(srcPtr);
    uint16* srcEnd = src + size / sizeof(uint16);
    while (src < srcEnd)
    {
        const uint16 in = *src;
        uint16 r = (in & 0xF800) >> 11;
        uint16 g = (in & 0x07C0) >> 1;
        uint16 b = (in & 0x003E) << 9;
        uint16 a = (in & 0x0001) << 15;

        *dst = r | g | b | a;

        ++dst;
        ++src;
    }
}

//------------------------------------------------------------------------------

inline void
_RGBA5551toABGR1555(void* srcPtr, void* dstPtr, uint32 size)
{
    uint16* dst = static_cast<uint16*>(dstPtr);
    uint16* src = static_cast<uint16*>(srcPtr);
    uint16* srcEnd = src + size / sizeof(uint16);
    while (src < srcEnd)
    {
        const uint16 in = *src;
        uint16 r = (in & 0x001F) << 11;
        uint16 g = (in & 0x03E0) << 1;
        uint16 b = (in & 0x7C00) >> 9;
        uint16 a = (in & 0x8000) >> 15;

        *dst = r | g | b | a;

        ++dst;
        ++src;
    }
}

//------------------------------------------------------------------------------

inline void
_SwapRB8(void* srcPtr, void* dstPtr, uint32 size)
{
    uint8* dst = static_cast<uint8*>(dstPtr);
    uint8* src = static_cast<uint8*>(srcPtr);
    uint8* srcEnd = src + size;
    while (src < srcEnd)
    {
        uint8 src0 = src[0];
        uint8 src2 = src[2];

        dst[0] = src2;
        dst[1] = src[1];
        dst[2] = src0;
        dst[3] = src[3];

        src += 4;
        dst += 4;
    }
}

//------------------------------------------------------------------------------

inline void
_SwapRB4(void* srcPtr, void* dstPtr, uint32 size)
{
    uint8* dst = static_cast<uint8*>(dstPtr);
    uint8* src = static_cast<uint8*>(srcPtr);
    uint8* srcEnd = src + size;
    while (src < srcEnd)
    {
        uint8 t0 = src[0];
        uint8 t1 = src[1];

        dst[0] = (t0 & 0xF0) | (t1 & 0x0F);
        dst[1] = (t1 & 0xF0) | (t0 & 0x0F);

        dst += 2;
        src += 2;
    }
}

//------------------------------------------------------------------------------

inline void
_SwapRB5551(void* srcPtr, void* dstPtr, uint32 size)
{
    uint8* dst = static_cast<uint8*>(dstPtr);
    uint8* src = static_cast<uint8*>(srcPtr);
    uint8* srcEnd = src + size;
    while (src < srcEnd)
    {
        uint8 t0 = src[0];
        uint8 t1 = src[1];

        dst[0] = ((t1 & 0x7C) >> 2) | (t0 & 0xE0);
        dst[1] = ((t0 & 0x1F) << 2) | (t1 & 0x83);

        dst += 2;
        src += 2;
    }
}

//------------------------------------------------------------------------------

} //namespace rhi

#endif //__RHI_FORMAT_CONVERT_H__