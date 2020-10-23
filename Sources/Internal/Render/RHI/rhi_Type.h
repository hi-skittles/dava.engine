#ifndef __RHI_TYPE_H__
#define __RHI_TYPE_H__

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Math/Math2D.h"

namespace DAVA
{
class File;
}

namespace rhi
{
using DAVA::uint8;
using DAVA::uint16;
using DAVA::uint32;
using DAVA::uint64;
using DAVA::int32;
using DAVA::float32;
using DAVA::Size2i;

typedef uint32 Handle;
static const uint32 InvalidHandle = 0;
static const uint32 DefaultDepthBuffer = static_cast<uint32>(-2);
static const uint64 NonreliableQueryValue = uint64(-1);

enum ResourceType
{
    RESOURCE_VERTEX_BUFFER = 11,
    RESOURCE_INDEX_BUFFER = 12,
    RESOURCE_QUERY_BUFFER = 13,
    RESOURCE_PERFQUERY = 14,
    RESOURCE_CONST_BUFFER = 22,
    RESOURCE_TEXTURE = 31,

    RESOURCE_PIPELINE_STATE = 41,
    RESOURCE_RENDER_PASS = 43,
    RESOURCE_COMMAND_BUFFER = 44,

    RESOURCE_DEPTHSTENCIL_STATE = 51,
    RESOURCE_SAMPLER_STATE = 52,

    RESOURCE_SYNC_OBJECT = 61,

    RESOURCE_PACKET_LIST = 100,
    RESOURCE_TEXTURE_SET = 101
};

enum Api
{
    RHI_DX11 = 0,
    RHI_DX9,
    RHI_GLES2,
    RHI_METAL,
    RHI_NULL_RENDERER,

    RHI_API_COUNT
};

enum class RenderingError : uint32_t
{
    FailedToCreateDevice,
    DriverError,
    UnsupportedShaderModel,
    FailedToInitialize
};

enum ProgType
{
    PROG_VERTEX,
    PROG_FRAGMENT
};

enum PrimitiveType
{
    PRIMITIVE_TRIANGLELIST = 1,
    PRIMITIVE_TRIANGLESTRIP = 2,
    PRIMITIVE_LINELIST = 10
};

enum FillMode
{
    FILLMODE_SOLID = 1,
    FILLMODE_WIREFRAME = 2
};

enum
{
    MAX_CONST_BUFFER_COUNT = 8,
    MAX_RENDER_TARGET_COUNT = 4,
    MAX_FRAGMENT_TEXTURE_SAMPLER_COUNT = 8,
    MAX_VERTEX_TEXTURE_SAMPLER_COUNT = 2,
    MAX_VERTEX_STREAM_COUNT = 4,
    MAX_SHADER_PROPERTY_COUNT = 1024,
    MAX_SHADER_CONST_BUFFER_COUNT = 1024,
};

//------------------------------------------------------------------------------
enum class AntialiasingType : DAVA::uint32
{
    NONE,
    MSAA_2X,
    MSAA_4X,
};

inline DAVA::uint32 TextureSampleCountForAAType(AntialiasingType type)
{
    switch (type)
    {
    case AntialiasingType::MSAA_2X:
        return 2;
    case AntialiasingType::MSAA_4X:
        return 4;
    default:
        return 1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// vertex-pipeline

enum VertexSemantics
{
    VS_POSITION = 1,
    VS_NORMAL = 2,
    VS_COLOR = 3,
    VS_TEXCOORD = 4,
    VS_TANGENT = 5,
    VS_BINORMAL = 6,
    VS_BLENDWEIGHT = 7,
    VS_BLENDINDEX = 8,

    VS_PAD = 100,

    VS_MAXCOUNT = 16
};

//------------------------------------------------------------------------------

inline const char*
VertexSemanticsName(VertexSemantics vs)
{
    switch (vs)
    {
    case VS_POSITION:
        return "position";
    case VS_NORMAL:
        return "normal";
    case VS_COLOR:
        return "color";
    case VS_TEXCOORD:
        return "texcoord";
    case VS_TANGENT:
        return "tangent";
    case VS_BINORMAL:
        return "binormal";
    case VS_BLENDWEIGHT:
        return "blend_weight";
    case VS_BLENDINDEX:
        return "blend_index";

    case VS_PAD:
        return "<pad>";
    case VS_MAXCOUNT:
        return "<max-count>";
    }

    return "<unknown>";
}

//------------------------------------------------------------------------------

enum VertexDataType
{
    VDT_FLOAT = 1,
    VDT_UINT8 = 2,
    VDT_INT16N = 3,
    VDT_INT8N = 4,
    VDT_UINT8N = 5,
    VDT_HALF = 6
};

//------------------------------------------------------------------------------

inline const char*
VertexDataTypeName(VertexDataType t)
{
    switch (t)
    {
    case VDT_FLOAT:
        return "float";
    case VDT_UINT8:
        return "uint8";
    case VDT_INT16N:
        return "int16n";
    case VDT_INT8N:
        return "int8n";
    case VDT_UINT8N:
        return "uint8n";
    case VDT_HALF:
        return "half";
    default:
        return "<unknown>";
    }
}

//------------------------------------------------------------------------------

enum VertexDataFrequency
{
    VDF_PER_VERTEX = 1,
    VDF_PER_INSTANCE = 2
};

//------------------------------------------------------------------------------

class VertexLayout
{
public:
    VertexLayout();
    ~VertexLayout();

    uint32 Stride(uint32 stream_i = 0) const;
    uint32 StreamCount() const;
    VertexDataFrequency StreamFrequency(uint32 stream_i) const;
    uint32 ElementCount() const;

    uint32 ElementStreamIndex(uint32 elem_i) const;
    VertexSemantics ElementSemantics(uint32 elem_i) const;
    uint32 ElementSemanticsIndex(uint32 elem_i) const;
    VertexDataType ElementDataType(uint32 elem_i) const;
    uint32 ElementDataCount(uint32 elem_i) const;
    uint32 ElementOffset(uint32 elem_i) const;
    uint32 ElementSize(uint32 elem_i) const;

    bool operator==(const VertexLayout& vl) const;
    VertexLayout& operator=(const VertexLayout& src);

    void Clear();
    void AddStream(VertexDataFrequency freq = VDF_PER_VERTEX);
    void AddElement(VertexSemantics usage, uint32 usage_i, VertexDataType type, uint32 dimension);
    void InsertElement(uint32 pos, VertexSemantics usage, uint32 usage_i, VertexDataType type, uint32 dimension);

    static bool IsCompatible(const VertexLayout& vbLayout, const VertexLayout& shaderLayout);
    static bool MakeCompatible(const VertexLayout& vbLayout, const VertexLayout& shaderLayout, VertexLayout* compatibleLayout);

    bool Save(DAVA::File* out) const;
    bool Load(DAVA::File* in);

    void Dump() const;

    static const VertexLayout* Get(uint32 uid);
    static uint32 UniqueId(const VertexLayout& layout);
    static const uint32 InvalidUID = 0;

private:
    enum
    {
        MaxElemCount = 8,
        MaxStreamCount = 2
    };

    struct
    Element
    {
        uint32 usage : 8;
        uint32 usage_index : 8;
        uint32 data_type : 8;
        uint32 data_count : 8;
    };

    struct
    Stream
    {
        uint32 first_elem : 8;
        uint32 elem_count : 8;
        uint32 freq : 8;
        uint32 __pad : 8;
    };

    Stream _stream[MaxStreamCount];
    uint32 _stream_count;

    Element _elem[MaxElemCount];
    uint32 _elem_count;
};

enum
{
    VATTR_POSITION_0 = 0,
    VATTR_NORMAL_0 = 1,
    VATTR_TEXCOORD_0 = 2,
    VATTR_TEXCOORD_1 = 4,
    VATTR_TEXCOORD_2 = 5,
    VATTR_TEXCOORD_3 = 6,
    VATTR_TEXCOORD_4 = 8,
    VATTR_TEXCOORD_5 = 9,
    VATTR_TEXCOORD_6 = 10,
    VATTR_TEXCOORD_7 = 11,
    VATTR_COLOR_0 = 3,
    VATTR_COLOR_1 = 7,
    VATTR_TANGENT = 12,
    VATTR_BINORMAL = 13,
    VATTR_BLENDWEIGHT = 14,
    VATTR_BLENDINDEX = 15,

    VATTR_POSITION_1 = 16,
    VATTR_POSITION_2 = 17,
    VATTR_POSITION_3 = 18,
    VATTR_NORMAL_1 = 19,
    VATTR_NORMAL_2 = 20,
    VATTR_NORMAL_3 = 21,

    VATTR_COUNT = 24
};

////////////////////////////////////////////////////////////////////////////////
// buffer

enum Usage
{
    USAGE_DEFAULT,
    USAGE_STATICDRAW,
    USAGE_DYNAMICDRAW
};

enum Pool
{
    POOL_DEFAULT,
    POOL_LOCALMEMORY,
    POOL_SYSTEMMEMORY
};

////////////////////////////////////////////////////////////////////////////////
// texture

enum TextureType
{
    TEXTURE_TYPE_1D,
    TEXTURE_TYPE_2D,
    TEXTURE_TYPE_CUBE
};

enum TextureFormat
{
    TEXTURE_FORMAT_R8G8B8A8 = 0,
    TEXTURE_FORMAT_R8G8B8X8,

    TEXTURE_FORMAT_R8G8B8,

    TEXTURE_FORMAT_R5G5B5A1,
    TEXTURE_FORMAT_R5G6B5,

    TEXTURE_FORMAT_R4G4B4A4,

    TEXTURE_FORMAT_A16R16G16B16,
    TEXTURE_FORMAT_A32R32G32B32,

    TEXTURE_FORMAT_R8,
    TEXTURE_FORMAT_R16,

    TEXTURE_FORMAT_DXT1,
    TEXTURE_FORMAT_DXT3,
    TEXTURE_FORMAT_DXT5,

    TEXTURE_FORMAT_PVRTC_4BPP_RGBA,
    TEXTURE_FORMAT_PVRTC_2BPP_RGBA,

    TEXTURE_FORMAT_PVRTC2_4BPP_RGB,
    TEXTURE_FORMAT_PVRTC2_4BPP_RGBA,
    TEXTURE_FORMAT_PVRTC2_2BPP_RGB,
    TEXTURE_FORMAT_PVRTC2_2BPP_RGBA,

    TEXTURE_FORMAT_ATC_RGB,
    TEXTURE_FORMAT_ATC_RGBA_EXPLICIT,
    TEXTURE_FORMAT_ATC_RGBA_INTERPOLATED,

    TEXTURE_FORMAT_ETC1,
    TEXTURE_FORMAT_ETC2_R8G8B8,
    TEXTURE_FORMAT_ETC2_R8G8B8A8,
    TEXTURE_FORMAT_ETC2_R8G8B8A1,

    TEXTURE_FORMAT_EAC_R11_UNSIGNED,
    TEXTURE_FORMAT_EAC_R11_SIGNED,
    TEXTURE_FORMAT_EAC_R11G11_UNSIGNED,
    TEXTURE_FORMAT_EAC_R11G11_SIGNED,

    TEXTURE_FORMAT_D16,
    TEXTURE_FORMAT_D24S8,

    TEXTURE_FORMAT_R16F,
    TEXTURE_FORMAT_RG16F,
    TEXTURE_FORMAT_RGBA16F,

    TEXTURE_FORMAT_R32F,
    TEXTURE_FORMAT_RG32F,
    TEXTURE_FORMAT_RGBA32F,
};

enum TextureFace
{
    TEXTURE_FACE_POSITIVE_X,
    TEXTURE_FACE_NEGATIVE_X,
    TEXTURE_FACE_POSITIVE_Y,
    TEXTURE_FACE_NEGATIVE_Y,
    TEXTURE_FACE_POSITIVE_Z,
    TEXTURE_FACE_NEGATIVE_Z,

    TEXTURE_FACE_NONE
};

enum TextureAddrMode
{
    TEXADDR_WRAP,
    TEXADDR_CLAMP,
    TEXADDR_MIRROR
};

enum TextureFilter
{
    TEXFILTER_NEAREST,
    TEXFILTER_LINEAR
};

enum TextureMipFilter
{
    TEXMIPFILTER_NONE,
    TEXMIPFILTER_NEAREST,
    TEXMIPFILTER_LINEAR
};

enum StencilOperation
{
    STENCILOP_KEEP,
    STENCILOP_ZERO,
    STENCILOP_REPLACE,
    STENCILOP_INVERT,
    STENCILOP_INCREMENT_CLAMP,
    STENCILOP_DECREMENT_CLAMP,
    STENCILOP_INCREMENT_WRAP,
    STENCILOP_DECREMENT_WRAP
};

enum LoadAction
{
    LOADACTION_NONE = 0,
    LOADACTION_CLEAR = 1,
    LOADACTION_LOAD = 2
};

enum StoreAction
{
    STOREACTION_NONE = 0,
    STOREACTION_STORE = 1,
    STOREACTION_RESOLVE = 2
};

namespace VertexBuffer
{
struct Descriptor
{
    uint32 size;
    Pool pool;
    Usage usage;
    const void* initialData;
    uint32 needRestore : 1;

    Descriptor(uint32 sz = 0)
        : size(sz)
        , pool(POOL_DEFAULT)
        , usage(USAGE_DEFAULT)
        , initialData(nullptr)
        , needRestore(true)
    {
    }
};
}

enum IndexSize
{
    INDEX_SIZE_16BIT = 0,
    INDEX_SIZE_32BIT = 1
};

namespace IndexBuffer
{
struct Descriptor
{
    uint32 size;
    IndexSize indexSize;
    Pool pool;
    Usage usage;
    const void* initialData;
    uint32 needRestore : 1;

    Descriptor(uint32 sz = 0)
        : size(sz)
        , indexSize(INDEX_SIZE_16BIT)
        , pool(POOL_DEFAULT)
        , usage(USAGE_DEFAULT)
        , initialData(nullptr)
        , needRestore(true)
    {
    }
};
}

namespace Texture
{
struct Descriptor
{
    TextureType type = TEXTURE_TYPE_2D;
    uint32 width = 0;
    uint32 height = 0;
    TextureFormat format = TEXTURE_FORMAT_R8G8B8A8;
    uint32 levelCount = 1;
    uint32 sampleCount = 1;
    void* initialData[128]; // it must be writable!
    uint32 isRenderTarget : 1;
    uint32 autoGenMipmaps : 1;
    uint32 needRestore : 1;
    uint32 cpuAccessRead : 1;
    uint32 cpuAccessWrite : 1;

    Descriptor(uint32 w, uint32 h, TextureFormat fmt)
        : width(w)
        , height(h)
        , format(fmt)
        , isRenderTarget(false)
        , autoGenMipmaps(false)
        , needRestore(true)
        , cpuAccessRead(false)
        , cpuAccessWrite(true)
    {
        memset(initialData, 0, sizeof(initialData));
    }

    Descriptor()
        : isRenderTarget(false)
        , autoGenMipmaps(false)
        , needRestore(true)
        , cpuAccessRead(false)
        , cpuAccessWrite(true)
    {
        memset(initialData, 0, sizeof(initialData));
    }
};
}

////////////////////////////////////////////////////////////////////////////////
// pipeline-state

enum ColorMask
{
    COLORMASK_NONE = 0,
    COLORMASK_R = (0x1 << 0),
    COLORMASK_G = (0x1 << 1),
    COLORMASK_B = (0x1 << 2),
    COLORMASK_A = (0x1 << 3),
    COLORMASK_ALL = COLORMASK_R | COLORMASK_G | COLORMASK_B | COLORMASK_A
};

enum BlendFunc
{
};

enum BlendOp
{
    BLENDOP_ZERO,
    BLENDOP_ONE,
    BLENDOP_SRC_ALPHA,
    BLENDOP_INV_SRC_ALPHA,
    BLENDOP_SRC_COLOR,
    BLENDOP_DST_COLOR
};

struct BlendState
{
    struct
    {
        uint32 colorFunc : 2;
        uint32 colorSrc : 3;
        uint32 colorDst : 3;
        uint32 alphaFunc : 2;
        uint32 alphaSrc : 3;
        uint32 alphaDst : 3;
        uint32 writeMask : 4;
        uint32 blendEnabled : 1;
        uint32 alphaToCoverage : 1;
    } rtBlend[MAX_RENDER_TARGET_COUNT];

    BlendState()
    {
        for (uint32 i = 0; i != MAX_RENDER_TARGET_COUNT; ++i)
        {
            rtBlend[i].colorFunc = 0;
            rtBlend[i].colorSrc = static_cast<uint32>(BLENDOP_ONE);
            rtBlend[i].colorDst = static_cast<uint32>(BLENDOP_ZERO);
            rtBlend[i].alphaFunc = 0;
            rtBlend[i].alphaSrc = static_cast<uint32>(BLENDOP_ONE);
            rtBlend[i].alphaDst = static_cast<uint32>(BLENDOP_ZERO);
            rtBlend[i].writeMask = COLORMASK_ALL;
            rtBlend[i].blendEnabled = false;
            rtBlend[i].alphaToCoverage = false;
        }
    }
};

namespace PipelineState
{
struct Descriptor
{
    VertexLayout vertexLayout;
    DAVA::FastName vprogUid;
    DAVA::FastName fprogUid;
    BlendState blending;
};
}

namespace SamplerState
{
struct Descriptor
{
    struct
    Sampler
    {
        uint32 addrU : 2;
        uint32 addrV : 2;
        uint32 addrW : 2;
        uint32 minFilter : 2;
        uint32 magFilter : 2;
        uint32 mipFilter : 2;
        uint32 anisotropyLevel : 8;
        uint32 pad : 12;

        Sampler()
            : addrU(TEXADDR_WRAP)
            , addrV(TEXADDR_WRAP)
            , addrW(TEXADDR_WRAP)
            , minFilter(TEXFILTER_LINEAR)
            , magFilter(TEXFILTER_LINEAR)
            , mipFilter(TEXMIPFILTER_LINEAR)
            , anisotropyLevel(1)
            , pad(0)
        {
        }
    };

    Sampler fragmentSampler[MAX_FRAGMENT_TEXTURE_SAMPLER_COUNT];
    uint32 fragmentSamplerCount;

    Sampler vertexSampler[MAX_VERTEX_TEXTURE_SAMPLER_COUNT];
    uint32 vertexSamplerCount;

    Descriptor()
        : fragmentSamplerCount(0)
        , vertexSamplerCount(0)
    {
        for (uint32 s = 0; s != MAX_VERTEX_TEXTURE_SAMPLER_COUNT; ++s)
        {
            vertexSampler[s].minFilter = TEXFILTER_NEAREST;
            vertexSampler[s].magFilter = TEXFILTER_NEAREST;
            vertexSampler[s].mipFilter = TEXMIPFILTER_NONE;
        }
    }
};
}

////////////////////////////////////////////////////////////////////////////////
// depth-stencil state

enum CmpFunc
{
    CMP_NEVER,
    CMP_LESS,
    CMP_EQUAL,
    CMP_LESSEQUAL,
    CMP_GREATER,
    CMP_NOTEQUAL,
    CMP_GREATEREQUAL,
    CMP_ALWAYS
};

namespace DepthStencilState
{
struct Descriptor
{
    uint32 depthTestEnabled : 1;
    uint32 depthWriteEnabled : 1;
    uint32 depthFunc : 3;

    uint32 stencilEnabled : 1;
    uint32 stencilTwoSided : 1;
    uint32 pad : 25;
    uint32 pad64 : 32;

    struct StencilDescriptor
    {
        uint8 readMask;
        uint8 writeMask;
        uint8 refValue;
        uint8 pad8;
        uint32 func : 3;
        uint32 failOperation : 3;
        uint32 depthFailOperation : 3;
        uint32 depthStencilPassOperation : 3;
        uint32 pad32 : 20;
    } stencilFront, stencilBack;

    Descriptor()
        : depthTestEnabled(true)
        , depthWriteEnabled(true)
        , depthFunc(CMP_LESSEQUAL)
        , stencilEnabled(false)
        , stencilTwoSided(false)
        , pad(0)
        , pad64(0)
    {
        stencilFront.readMask = 0xFF;
        stencilFront.writeMask = 0xFF;
        stencilFront.refValue = 0;
        stencilFront.func = CMP_ALWAYS;
        stencilFront.failOperation = STENCILOP_KEEP;
        stencilFront.depthFailOperation = STENCILOP_KEEP;
        stencilFront.depthStencilPassOperation = STENCILOP_KEEP;
        stencilFront.pad8 = 0;
        stencilFront.pad32 = 0;

        stencilBack.readMask = 0xFF;
        stencilBack.writeMask = 0xFF;
        stencilBack.refValue = 0;
        stencilBack.func = CMP_ALWAYS;
        stencilBack.failOperation = STENCILOP_KEEP;
        stencilBack.depthFailOperation = STENCILOP_KEEP;
        stencilBack.depthStencilPassOperation = STENCILOP_KEEP;
        stencilBack.pad8 = 0;
        stencilBack.pad32 = 0;
    }
};
}

struct ProgConstInfo
{
    DAVA::FastName uid; // name
    uint32 bufferIndex;
    uint32 offset; // from start of buffer
    int type; // size deduced from type -- float4 = 4*sizeof(float) etc.
};

////////////////////////////////////////////////////////////////////////////////
// cull-mode

enum CullMode
{
    CULL_NONE = 0,
    CULL_CCW = 1,
    CULL_CW = 2
};

////////////////////////////////////////////////////////////////////////////////
// viewport

struct Viewport
{
    uint32 x = 0;
    uint32 y = 0;
    uint32 width = 0;
    uint32 height = 0;

    Viewport() = default;

    Viewport(uint32 x_, uint32 y_, uint32 w_, uint32 h_)
        : x(x_)
        , y(y_)
        , width(w_)
        , height(h_)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////
// render-target state

struct RenderPassConfig
{
    struct ColorBuffer
    {
        Handle texture = InvalidHandle;
        Handle multisampleTexture = InvalidHandle;
        TextureFace textureFace = TEXTURE_FACE_NONE;
        uint32 textureLevel = 0;
        LoadAction loadAction = LOADACTION_CLEAR;
        StoreAction storeAction = STOREACTION_NONE;
        float clearColor[4];

        ColorBuffer()
        {
            clearColor[0] = 0;
            clearColor[1] = 0;
            clearColor[2] = 0;
            clearColor[3] = 1.0f;
        }
    };

    struct DepthStencilBuffer
    {
        Handle texture = DefaultDepthBuffer;
        Handle multisampleTexture = InvalidHandle;
        LoadAction loadAction = LOADACTION_CLEAR;
        StoreAction storeAction = STOREACTION_NONE;
        float clearDepth = 1.0f;
        uint32 clearStencil = 0;
    };

    ColorBuffer colorBuffer[MAX_RENDER_TARGET_COUNT];
    DepthStencilBuffer depthStencilBuffer;

    AntialiasingType antialiasingType = AntialiasingType::NONE;

    Handle queryBuffer = InvalidHandle;
    Handle perfQueryStart = InvalidHandle;
    Handle perfQueryEnd = InvalidHandle;
    Viewport viewport;
    int32 priority = 0;
    uint32 invertCulling = 0;

    bool IsValid() const
    {
        if (depthStencilBuffer.storeAction == STOREACTION_RESOLVE)
            return false;

        bool usingResolve = colorBuffer[0].storeAction == STOREACTION_RESOLVE;
        bool hasMSTexture = colorBuffer[0].multisampleTexture != InvalidHandle;

        if (UsingMSAA() && !(usingResolve || hasMSTexture))
            return false;

        if (usingResolve && !(UsingMSAA() || hasMSTexture))
            return false;

        return true;
    }

    bool UsingMSAA() const
    {
        return (antialiasingType == AntialiasingType::MSAA_2X) || (antialiasingType == AntialiasingType::MSAA_4X);
    }
};

////////////////////////////////////////////////////////////////////////////////
// query-buffer

namespace QueryBuffer
{
struct
Descriptor
{
};
}

////////////////////////////////////////////////////////////////////////////////
// sync-object

namespace SyncObject
{
struct
Descriptor
{
};
}

////////////////////////////////////////////////////////////////////////////////
// command-buffer

namespace CommandBuffer
{
struct
Descriptor
{
};
}

///////////////////////////////////////////////////////////////////////////////
// perf-query
namespace PerfQuery
{
struct Descriptor
{
};
}

///////////////////////////////////////////////////////////////////////////////
// const-buffer
namespace ConstBuffer
{
struct Descriptor
{
};
}

//------------------------------------------------------------------------------

struct
ScissorRect
{
    uint16 x;
    uint16 y;
    uint16 width;
    uint16 height;

    ScissorRect()
        : x(0)
        , y(0)
        , width(0)
        , height(0)
    {
    }
};

} // namespace rhi

//------------------------------------------------------------------------------
//
//








#endif // __RHI_TYPE_H__
