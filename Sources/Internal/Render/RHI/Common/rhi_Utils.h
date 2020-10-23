#pragma once

#include "../rhi_Type.h"

template <typename src, typename dst>
inline dst nonaliased_cast(src x)
{
    // (quite a funny way to) ensure types are the same size
    // commented-out until acceptable way to stop the compiler-spamming is found
    //    #pragma warning( disable: 177 ) // variable "WrongSizes" was declared but never referenced
    //    static int  WrongSizes[sizeof(src) == sizeof(dst) ? 1 : -1];
    //    #pragma warning( default: 177 )

    union {
        src s;
        dst d;
    } tmp;

    tmp.s = x;

    return tmp.d;
}

#define countof(array) (sizeof(array) / sizeof(array[0]))
#define L_ALIGNED_SIZE(size, align) (((size) + ((align)-1)) & (~((align)-1)))

inline bool IsEmptyString(const char* str)
{
    return !(str && str[0] != '\0');
}

void Trace(const char* format, ...);
#define LCP Trace("%s : %i\n", __FILE__, __LINE__);

namespace rhi
{
Size2i TextureExtents(Size2i size, uint32 level);
uint32 TextureStride(TextureFormat format, Size2i size, uint32 level);
uint32 TextureSize(TextureFormat format, uint32 width, uint32 height, uint32 level = 0);
}
