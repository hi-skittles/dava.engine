#ifndef __DAVAENGINE_RENDER_BASE_H__
#define __DAVAENGINE_RENDER_BASE_H__

#include "Base/BaseTypes.h"
#include "Base/GlobalEnum.h"
#include "DAVAConfig.h"
#include "Base/FastName.h"
#include "Render/RHI/rhi_Type.h"

namespace DAVA
{
enum eBlending
{
    BLENDING_NONE = 0,
    BLENDING_ALPHABLEND,
    BLENDING_ADDITIVE,
    BLENDING_ALPHA_ADDITIVE,
    BLENDING_SOFT_ADDITIVE,
    BLENDING_MULTIPLICATIVE,
    BLENDING_STRONG_MULTIPLICATIVE,
    BLENDING_PREMULTIPLIED_ALPHA,
};

enum eGradientBlendMode
{
    GRADIENT_MULTIPLY = 0,
    GRADIENT_BLEND,
    GRADIENT_ADD,
    GRADIENT_SCREEN,
    GRADIENT_OVERLAY,
    GRADIENT_BLEND_MODE_COUNT
};

enum ImageQuality : uint8
{
    MIN_IMAGE_QUALITY = 0,
    MAX_IMAGE_QUALITY = 100,
    LOSSLESS_IMAGE_QUALITY = 255,
    DEFAULT_IMAGE_QUALITY = MAX_IMAGE_QUALITY
};

enum ImageFormat : uint8
{
    IMAGE_FORMAT_PNG = 0,
    IMAGE_FORMAT_DDS,
    IMAGE_FORMAT_PVR,
    IMAGE_FORMAT_JPEG,
    IMAGE_FORMAT_TGA,
    IMAGE_FORMAT_WEBP,
    IMAGE_FORMAT_PSD,
    IMAGE_FORMAT_HDR,
    IMAGE_FORMAT_COUNT,
    IMAGE_FORMAT_UNKNOWN = 127
};

enum PixelFormat : uint8
{
    FORMAT_INVALID = 0,
    FORMAT_RGBA8888 = 1,
    FORMAT_RGBA5551,
    FORMAT_RGBA4444,
    FORMAT_RGB888,
    FORMAT_RGB565,
    FORMAT_A8,
    FORMAT_A16,
    FORMAT_PVR4,
    FORMAT_PVR2,

    FORMAT_RGBA16161616,
    FORMAT_RGBA32323232,

    FORMAT_DXT1,
    FORMAT_REMOVED_DXT_1N, //to use it in
    FORMAT_DXT1A = 14, //back compatibility
    FORMAT_DXT3,
    FORMAT_DXT5,
    FORMAT_DXT5NM,

    FORMAT_ETC1,

    FORMAT_ATC_RGB,
    FORMAT_ATC_RGBA_EXPLICIT_ALPHA,
    FORMAT_ATC_RGBA_INTERPOLATED_ALPHA,

    FORMAT_PVR2_2, //pvrtc2 generation
    FORMAT_PVR4_2,
    FORMAT_EAC_R11_UNSIGNED,
    FORMAT_EAC_R11_SIGNED,
    FORMAT_EAC_RG11_UNSIGNED, //2 channels format for normal maps
    FORMAT_EAC_RG11_SIGNED, //2 channels format for normal maps
    FORMAT_ETC2_RGB,
    FORMAT_ETC2_RGBA,
    FORMAT_ETC2_RGB_A1,

    FORMAT_BGR888, // windows BMP format
    FORMAT_BGRA8888, // android web view format only for ImageConvert

    FORMAT_R16F,
    FORMAT_RG16F,
    FORMAT_RGB16F,
    FORMAT_RGBA16F,
    FORMAT_R32F,
    FORMAT_RG32F,
    FORMAT_RGB32F,
    FORMAT_RGBA32F,

    FORMAT_COUNT,
    FORMAT_CLOSEST = 255 // fit PixelFormat at 8bits (PixelFormat format:8;)
};

// Please update JniDeviceInfo.java if change eGPUFamily enum
enum eGPUFamily : uint8
{
    GPU_POWERVR_IOS = 0,
    GPU_POWERVR_ANDROID,
    GPU_TEGRA,
    GPU_MALI,
    GPU_ADRENO,
    GPU_DX11,
    GPU_ORIGIN, // not a device - for development only
    GPU_FAMILY_COUNT,

    GPU_DEVICE_COUNT = GPU_ORIGIN,
    GPU_INVALID = 127
};

enum eIndexFormat
{
    EIF_16 = 0x0,
    EIF_32 = 0x1,
};

static const int32 INDEX_FORMAT_SIZE[2] = { 2, 4 };

const int32 STENCILOP_COUNT = 8; //rhi::StencilOperation
const int32 CMP_TEST_MODE_COUNT = 8; //rhi::CmpFunc
const int32 FILLMODE_COUNT = 3;
extern const String CMP_FUNC_NAMES[CMP_TEST_MODE_COUNT];
extern const String STENCIL_OP_NAMES[STENCILOP_COUNT];

extern const String FILL_MODE_NAMES[FILLMODE_COUNT];

enum eDefaultPassPriority
{
    PRIORITY_MAIN_2D = 00,
    PRIORITY_MAIN_3D = 10,
    PRIORITY_CLEAR = 15,
    PRIORITY_SERVICE_3D = 20,
    PRIORITY_SCREENSHOT = 50,
    PRIORITY_SCREENSHOT_2D = PRIORITY_SCREENSHOT, //legacy

    PRIORITY_SERVICE_2D = 1000 //service 2d is most commonly used for system's draw to texture etc.
};

// TODO: we have same structs & functions in PolygonGroup -- we should find a right place for them
enum eVertexFormat
{
    EVF_VERTEX = 1,
    EVF_NORMAL = 1 << 1,
    EVF_COLOR = 1 << 2,
    EVF_TEXCOORD0 = 1 << 3,
    EVF_TEXCOORD1 = 1 << 4,
    EVF_TEXCOORD2 = 1 << 5,
    EVF_TEXCOORD3 = 1 << 6,
    EVF_TANGENT = 1 << 7,
    EVF_BINORMAL = 1 << 8,
    EVF_HARD_JOINTINDEX = 1 << 9, //single joint index for hard-skinning
    EVF_PIVOT4 = 1 << 10,
    EVF_PIVOT_DEPRECATED = 1 << 11, //deprecated, need remove after content re-saving
    EVF_FLEXIBILITY = 1 << 12,
    EVF_ANGLE_SIN_COS = 1 << 13,
    EVF_JOINTINDEX = 1 << 14,
    EVF_JOINTWEIGHT = 1 << 15,
    EVF_CUBETEXCOORD0 = 1 << 16,
    EVF_CUBETEXCOORD1 = 1 << 17,
    EVF_CUBETEXCOORD2 = 1 << 18,
    EVF_CUBETEXCOORD3 = 1 << 19,

    EVF_LOWER_BIT = EVF_VERTEX,
    EVF_HIGHER_BIT = EVF_JOINTWEIGHT, // shouldn't it be EVF_CUBETEXCOORD3?
    EVF_NEXT_AFTER_HIGHER_BIT = (EVF_HIGHER_BIT << 1),
    EVF_FORCE_DWORD = 0x7fffffff,
};
enum
{
    VERTEX_FORMAT_STREAM_MAX_COUNT = 16
};

inline int32 GetTexCoordCount(int32 vertexFormat)
{
    int32 ret = 0;
    ret += (vertexFormat & EVF_TEXCOORD0) ? 1 : 0;
    ret += (vertexFormat & EVF_TEXCOORD1) ? 1 : 0;
    ret += (vertexFormat & EVF_TEXCOORD2) ? 1 : 0;
    ret += (vertexFormat & EVF_TEXCOORD3) ? 1 : 0;
    return ret;
}

inline int32 GetCubeTexCoordCount(int32 vertexFormat)
{
    int32 ret = 0;
    for (int32 i = EVF_CUBETEXCOORD0; i < EVF_CUBETEXCOORD3 + 1; i = (i << 1))
    {
        if (vertexFormat & i)
        {
            ret++;
        }
    }

    return ret;
}

inline int32 GetVertexSize(int32 flags)
{
    int32 size = 0;
    if (flags & EVF_VERTEX)
        size += 3 * sizeof(float32);
    if (flags & EVF_NORMAL)
        size += 3 * sizeof(float32);
    if (flags & EVF_COLOR)
        size += 4;
    if (flags & EVF_TEXCOORD0)
        size += 2 * sizeof(float32);
    if (flags & EVF_TEXCOORD1)
        size += 2 * sizeof(float32);
    if (flags & EVF_TEXCOORD2)
        size += 2 * sizeof(float32);
    if (flags & EVF_TEXCOORD3)
        size += 2 * sizeof(float32);

    if (flags & EVF_TANGENT)
        size += 3 * sizeof(float32);
    if (flags & EVF_BINORMAL)
        size += 3 * sizeof(float32);
    if (flags & EVF_HARD_JOINTINDEX)
        size += sizeof(float32);

    if (flags & EVF_CUBETEXCOORD0)
        size += 3 * sizeof(float32);
    if (flags & EVF_CUBETEXCOORD1)
        size += 3 * sizeof(float32);
    if (flags & EVF_CUBETEXCOORD2)
        size += 3 * sizeof(float32);
    if (flags & EVF_CUBETEXCOORD3)
        size += 3 * sizeof(float32);

    if (flags & EVF_PIVOT4)
        size += 4 * sizeof(float32);
    if (flags & EVF_PIVOT_DEPRECATED)
        size += 3 * sizeof(float32);
    if (flags & EVF_FLEXIBILITY)
        size += sizeof(float32);
    if (flags & EVF_ANGLE_SIN_COS)
        size += 2 * sizeof(float32);

    if (flags & EVF_JOINTINDEX)
        size += 4 * sizeof(float32);
    if (flags & EVF_JOINTWEIGHT)
        size += 4 * sizeof(float32);

    return size;
}

inline uint32 CalculatePrimitiveCount(uint32 indexCount, rhi::PrimitiveType primitiveType)
{
    switch (primitiveType)
    {
    case rhi::PRIMITIVE_TRIANGLELIST:
        return indexCount / 3;
    case rhi::PRIMITIVE_LINELIST:
        return indexCount / 2;
    case rhi::PRIMITIVE_TRIANGLESTRIP:
        return indexCount - 2;
    default:
        DVASSERT(false, "Unknown primitive type");
    }
    return 0;
}

uint32 GetVertexLayoutRequiredFormat(const rhi::VertexLayout& layout);

rhi::CmpFunc GetCmpFuncByName(const String& cmpFuncStr);
rhi::StencilOperation GetStencilOpByName(const String& stencilOpStr);
};

#endif // __DAVAENGINE_RENDER_BASE_H__
