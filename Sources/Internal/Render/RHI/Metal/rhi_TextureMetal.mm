#include "../Common/rhi_Private.h"
#include "../Common/rhi_Utils.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_FormatConversion.h"
#include "rhi_Metal.h"

#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
using DAVA::Logger;

#include "_metal.h"

#if !(TARGET_IPHONE_SIMULATOR == 1)

#define RHI_METAL__USE_PURGABLE_STATE 0

namespace rhi
{
//==============================================================================

struct
TextureMetal_t
: public ResourceImpl<TextureMetal_t, Texture::Descriptor>
{
public:
    TextureMetal_t()
        : mappedData(nullptr)
        , width(0)
        , height(0)
        , mappedDataSize(0)
        , uid(nil)
        , uid2(nil)
        , is_mapped(false)
        , is_renderable(false)
        , is_cubemap(false)
    {
    }

    TextureFormat format;
    uint32 width;
    uint32 height;
    void* mappedData;
    uint32 mappedDataSize;
    uint32 mappedLevel;
    uint32 mappedSlice;
    id<MTLTexture> uid;
    id<MTLTexture> uid2;
    Texture::Descriptor creationDesc;
    uint32 is_mapped : 1;
    uint32 is_renderable : 1;
    uint32 is_cubemap : 1;
    uint32 need_restoring : 1;
};
RHI_IMPL_RESOURCE(TextureMetal_t, Texture::Descriptor);

typedef ResourcePool<TextureMetal_t, RESOURCE_TEXTURE, Texture::Descriptor, true> TextureMetalPool;
RHI_IMPL_POOL(TextureMetal_t, RESOURCE_TEXTURE, Texture::Descriptor, true);

static void
_CheckAllTextures()
{
    bool first = true;
    for (TextureMetalPool::Iterator t = TextureMetalPool::Begin(), t_end = TextureMetalPool::End(); t != t_end; ++t)
    {
        if ([t->uid setPurgeableState:MTLPurgeableStateKeepCurrent] == MTLPurgeableStateEmpty)
        {
            if (!t->NeedRestore())
            {
                t->MarkNeedRestore();
                DAVA::Logger::Info("tex-lost  %ux%u  ps= %i", t->width, t->height, int([t->uid setPurgeableState:MTLPurgeableStateKeepCurrent]));
            }
            /*
            if (first)
            {
                DAVA::Logger::Info("--");
                first = false;
            }
            DAVA::Logger::Info("tex-lost  %ux%u  ps= %i", t->width, t->height, int([t->uid setPurgeableState:MTLPurgeableStateKeepCurrent]));
*/
        }
    }
}

//------------------------------------------------------------------------------

static MTLPixelFormat
MetalTextureFormat(TextureFormat format)
{
    switch (format)
    {
    //        case TEXTURE_FORMAT_A8R8G8B8    : return MTLPixelFormatBGRA8Unorm;
    case TEXTURE_FORMAT_R8G8B8A8:
        return MTLPixelFormatRGBA8Unorm;
    //        TEXTURE_FORMAT_X8R8G8B8,

    case TEXTURE_FORMAT_R5G5B5A1:
        return MTLPixelFormatA1BGR5Unorm;
    case TEXTURE_FORMAT_R5G6B5:
        return MTLPixelFormatB5G6R5Unorm;

    case TEXTURE_FORMAT_R4G4B4A4:
        return MTLPixelFormatABGR4Unorm;

    //        TEXTURE_FORMAT_A16R16G16B16,
    //        TEXTURE_FORMAT_A32R32G32B32,

    case TEXTURE_FORMAT_R8:
        return MTLPixelFormatA8Unorm;
    //        TEXTURE_FORMAT_R16,

    case TEXTURE_FORMAT_PVRTC_4BPP_RGBA:
        return MTLPixelFormatPVRTC_RGBA_4BPP;
    case TEXTURE_FORMAT_PVRTC_2BPP_RGBA:
        return MTLPixelFormatPVRTC_RGBA_2BPP;
    /*
    case TEXTURE_FORMAT_PVRTC2_4BPP_RGB:
        return MTLPixelFormatPVRTC_RGB_4BPP;
    case TEXTURE_FORMAT_PVRTC2_4BPP_RGBA:
        return MTLPixelFormatPVRTC_RGBA_4BPP;
    case TEXTURE_FORMAT_PVRTC2_2BPP_RGB:
        return MTLPixelFormatPVRTC_RGB_2BPP;
    case TEXTURE_FORMAT_PVRTC2_2BPP_RGBA:
        return MTLPixelFormatPVRTC_RGBA_2BPP;
*/
    //        TEXTURE_FORMAT_ATC_RGB,
    //        TEXTURE_FORMAT_ATC_RGBA_EXPLICIT,
    //        TEXTURE_FORMAT_ATC_RGBA_INTERPOLATED,

    //        TEXTURE_FORMAT_ETC1,
    case TEXTURE_FORMAT_ETC2_R8G8B8:
        return MTLPixelFormatETC2_RGB8;
    //        case TEXTURE_FORMAT_ETC2_R8G8B8A8       : pf = MTLPixelFormatETC2_RGBA8; break;
    case TEXTURE_FORMAT_ETC2_R8G8B8A1:
        return MTLPixelFormatETC2_RGB8A1;

    case TEXTURE_FORMAT_EAC_R11_UNSIGNED:
        return MTLPixelFormatEAC_R11Unorm;
    case TEXTURE_FORMAT_EAC_R11_SIGNED:
        return MTLPixelFormatEAC_R11Snorm;
    //        case TEXTURE_FORMAT_EAC_R11G11_UNSIGNED : pf = MTLPixelFormatEAC_R11G11Unorm; break;
    //        case TEXTURE_FORMAT_EAC_R11G11_SIGNED   : pf = MTLPixelFormatEAC_R11G11Snorm; break;

    case TEXTURE_FORMAT_D24S8:
    case TEXTURE_FORMAT_D16:
        return MTLPixelFormatDepth32Float;

    case TEXTURE_FORMAT_R16F:
        return MTLPixelFormatR16Float;
    case TEXTURE_FORMAT_R32F:
        return MTLPixelFormatR32Float;
    case TEXTURE_FORMAT_RG16F:
        return MTLPixelFormatRG16Float;
    case TEXTURE_FORMAT_RG32F:
        return MTLPixelFormatRG32Float;
    case TEXTURE_FORMAT_RGBA16F:
        return MTLPixelFormatRGBA16Float;
    case TEXTURE_FORMAT_RGBA32F:
        return MTLPixelFormatRGBA32Float;

    default:
        return MTLPixelFormatInvalid;
    }
}

static MTLPixelFormat
MetalRenderableTextureFormat(TextureFormat format)
{
    switch (format)
    {
    case TEXTURE_FORMAT_R8G8B8A8:
        return MTLPixelFormatBGRA8Unorm;
    case TEXTURE_FORMAT_R8:
        return MTLPixelFormatR8Unorm;
    case TEXTURE_FORMAT_R16:
        return MTLPixelFormatR16Unorm;
    case TEXTURE_FORMAT_R5G6B5:
        return MTLPixelFormatB5G6R5Unorm;
    case TEXTURE_FORMAT_R5G5B5A1:
        return MTLPixelFormatA1BGR5Unorm;
    case TEXTURE_FORMAT_R4G4B4A4:
        return MTLPixelFormatABGR4Unorm;
    case TEXTURE_FORMAT_D16:
        return MTLPixelFormatDepth32Float;
    case TEXTURE_FORMAT_R16F:
        return MTLPixelFormatR16Float;
    case TEXTURE_FORMAT_R32F:
        return MTLPixelFormatR32Float;
    case TEXTURE_FORMAT_RG16F:
        return MTLPixelFormatRG16Float;
    case TEXTURE_FORMAT_RG32F:
        return MTLPixelFormatRG32Float;
    case TEXTURE_FORMAT_RGBA16F:
        return MTLPixelFormatRGBA16Float;
    case TEXTURE_FORMAT_RGBA32F:
        return MTLPixelFormatRGBA32Float;
    case TEXTURE_FORMAT_D24S8:
        return MTLPixelFormatDepth32Float;
    default:
    {
        DAVA::Logger::Error("Invalid or unsupported renderable format requested: %u", static_cast<DAVA::uint32>(format));
        return MTLPixelFormatRGBA8Unorm;
    }
    }
}

//------------------------------------------------------------------------------

static bool
_Construct(TextureMetal_t* tex, const Texture::Descriptor& texDesc)
{
    tex->UpdateCreationDesc(texDesc);

    bool success = true;
    MTLPixelFormat pf = (texDesc.isRenderTarget) ? MetalRenderableTextureFormat(texDesc.format) : MetalTextureFormat(texDesc.format);

    //    MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pf width:texDesc.width height:texDesc.height mipmapped:NO];
    MTLTextureDescriptor* desc = [MTLTextureDescriptor new];

    if (!desc)
    {
        Logger::Info("failed to create desc for tex%s %ux%u fmt=%i", (texDesc.isRenderTarget) ? "-rt" : "", texDesc.width, texDesc.height, int(texDesc.format));
        return InvalidHandle;
    }
    desc.width = texDesc.width;
    desc.height = texDesc.height;
    desc.pixelFormat = pf;
    desc.arrayLength = 1;
    desc.sampleCount = texDesc.sampleCount;
    desc.mipmapLevelCount = texDesc.levelCount;
    desc.resourceOptions = MTLResourceCPUCacheModeDefaultCache;
    if (texDesc.type == TEXTURE_TYPE_CUBE)
    {
        DVASSERT(texDesc.sampleCount == 1);
        desc.textureType = MTLTextureTypeCube;
    }
    else
    {
        desc.textureType = (texDesc.sampleCount > 1) ? MTLTextureType2DMultisample : MTLTextureType2D;
    }

    id<MTLTexture> uid = [_Metal_Device newTextureWithDescriptor:desc];

    [desc release];

    if (uid != nil)
    {
        [uid retain];

        tex->format = texDesc.format;
        tex->width = texDesc.width;
        tex->height = texDesc.height;
        tex->uid = uid;
        tex->is_mapped = false;
        tex->is_renderable = texDesc.isRenderTarget;
        tex->is_cubemap = texDesc.type == TEXTURE_TYPE_CUBE;

        uint32 sliceCount = (texDesc.type == TEXTURE_TYPE_CUBE) ? 6 : 1;

        for (unsigned s = 0; s != sliceCount; ++s)
        {
            for (unsigned m = 0; m != texDesc.levelCount; ++m)
            {
                void* data = texDesc.initialData[s * texDesc.levelCount + m];

                if (data)
                {
                    MTLRegion rgn;
                    uint32 stride = TextureStride(texDesc.format, Size2i(texDesc.width, texDesc.height), m);
                    Size2i ext = TextureExtents(Size2i(texDesc.width, texDesc.height), m);
                    unsigned sz = TextureSize(texDesc.format, texDesc.width, texDesc.height, m);

                    rgn.origin.x = 0;
                    rgn.origin.y = 0;
                    rgn.origin.z = 0;
                    rgn.size.width = ext.dx;
                    rgn.size.height = ext.dy;
                    rgn.size.depth = 1;

                    if (texDesc.format == TEXTURE_FORMAT_R4G4B4A4)
                        _FlipRGBA4_ABGR4(texDesc.initialData[m], texDesc.initialData[m], sz);
                    else if (texDesc.format == TEXTURE_FORMAT_R5G5B5A1)
                        _ABGR1555toRGBA5551(texDesc.initialData[m], texDesc.initialData[m], sz);

                    if ((texDesc.format == TEXTURE_FORMAT_PVRTC_4BPP_RGBA) || (texDesc.format == TEXTURE_FORMAT_PVRTC_2BPP_RGBA))
                    {
                        stride = 0;
                        sz = 0;
                    }

                    [uid replaceRegion:rgn mipmapLevel:m slice:s withBytes:data bytesPerRow:stride bytesPerImage:sz];
                }
                else
                {
                    break;
                }
            }
        }
        #if RHI_METAL__USE_PURGABLE_STATE
        [tex->uid setPurgeableState:MTLPurgeableStateNonVolatile];
        #endif

        tex->mappedDataSize = TextureSize(texDesc.format, texDesc.width, texDesc.height, 0);

        if (texDesc.format == TEXTURE_FORMAT_D24S8)
        {
            MTLPixelFormat pf2 = MTLPixelFormatStencil8;
            MTLTextureDescriptor* desc2 = [MTLTextureDescriptor new];
            desc2.pixelFormat = pf2;
            desc2.width = texDesc.width;
            desc2.height = texDesc.height;
            desc2.mipmapLevelCount = 1;
            desc2.sampleCount = texDesc.sampleCount;
            desc2.textureType = texDesc.sampleCount > 1 ? MTLTextureType2DMultisample : MTLTextureType2D;

            id<MTLTexture> uid2 = [_Metal_Device newTextureWithDescriptor:desc2];

            if (uid2)
            {
                #if RHI_METAL__USE_PURGABLE_STATE
                [tex->uid2 setPurgeableState:MTLPurgeableStateNonVolatile];
                #endif
                tex->uid2 = uid2;
                uid2 = nil;
            }
            else
            {
                success = false;
            }

            [desc2 release];
        }
    }
    else
    {
        DAVA::Logger::Debug("failed to create tex%s %ux%u fmt=%i", (texDesc.isRenderTarget) ? "-rt" : "", texDesc.width, texDesc.height, int(texDesc.format));
        success = false;
    }

    tex->need_restoring = texDesc.needRestore;
    tex->creationDesc = texDesc;

    return success;
}

//------------------------------------------------------------------------------

static void
_Destroy(TextureMetal_t* self)
{
    //Logger::Info("{%u} del-tex%s %ux%u", unsigned(RHI_HANDLE_INDEX(tex)), (self->is_renderable) ? "-rt" : "",self->width,self->height);
    if (self->mappedData)
    {
        ::free(self->mappedData);
        self->mappedData = nullptr;
    }

    if (self->uid)
    {
        #if RHI_METAL__USE_PURGABLE_STATE
        [self->uid setPurgeableState:MTLPurgeableStateVolatile];
        #endif
        [self->uid release];
        [self->uid release];
        self->uid = nil;
    }
    if (self->uid2)
    {
        #if RHI_METAL__USE_PURGABLE_STATE
        [self->uid2 setPurgeableState:MTLPurgeableStateVolatile];
        #endif
        [self->uid2 release];
        self->uid2 = nil;
    }

    self->MarkRestored();
}

//------------------------------------------------------------------------------

static Handle
metal_Texture_Create(const Texture::Descriptor& texDesc)
{
    DVASSERT(texDesc.levelCount);

    if (
    (texDesc.format == TEXTURE_FORMAT_PVRTC_4BPP_RGBA ||
     texDesc.format == TEXTURE_FORMAT_PVRTC_2BPP_RGBA ||
     texDesc.format == TEXTURE_FORMAT_PVRTC2_4BPP_RGB ||
     texDesc.format == TEXTURE_FORMAT_PVRTC2_4BPP_RGBA ||
     texDesc.format == TEXTURE_FORMAT_PVRTC2_2BPP_RGB ||
     texDesc.format == TEXTURE_FORMAT_PVRTC2_2BPP_RGBA)
    && (texDesc.width != texDesc.height)
    )
    {
        Logger::Error("can't create non-square PVRTC-tex %ux%u fmt=%i", texDesc.width, texDesc.height, int(texDesc.format));
        return InvalidHandle;
    }

    Handle handle = TextureMetalPool::Alloc();
    TextureMetal_t* tex = TextureMetalPool::Get(handle);

    if (_Construct(tex, texDesc))
    {
    }
    else
    {
        DAVA::Logger::Debug("failed to create tex%s %ux%u fmt=%i", (texDesc.isRenderTarget) ? "-rt" : "", texDesc.width, texDesc.height, int(texDesc.format));

        TextureMetalPool::Free(handle);
        handle = InvalidHandle;
    }

    //_CheckAllTextures();
    return handle;
}

//------------------------------------------------------------------------------

static void
metal_Texture_Delete(Handle tex, bool)
{
    TextureMetal_t* self = TextureMetalPool::Get(tex);

    if (self)
    {
        _Destroy(self);
        TextureMetalPool::Free(tex);
    }
    else
    {
        DVASSERT("kaboom!!!");
    }
    //_CheckAllTextures();
}

//------------------------------------------------------------------------------

static void*
metal_Texture_Map(Handle tex, unsigned level, TextureFace face)
{
    TextureMetal_t* self = TextureMetalPool::Get(tex);
    MTLRegion rgn;
    uint32 stride = TextureStride(self->format, Size2i([self->uid width], [self->uid height]), level);
    Size2i ext = TextureExtents(Size2i([self->uid width], [self->uid height]), level);
    unsigned sz = TextureSize(self->format, [self->uid width], [self->uid height], level);

    ///    DVASSERT(!self->is_renderable);
    DVASSERT(!self->is_mapped);

    rgn.origin.x = 0;
    rgn.origin.y = 0;
    rgn.origin.z = 0;
    rgn.size.width = ext.dx;
    rgn.size.height = ext.dy;
    rgn.size.depth = 1;

    if (!self->mappedData)
        self->mappedData = ::malloc(self->mappedDataSize);

    if (self->is_cubemap)
    {
        NSUInteger slice = 0;

        switch (face)
        {
        case TEXTURE_FACE_NEGATIVE_X:
            slice = 0;
            break;
        case TEXTURE_FACE_POSITIVE_X:
            slice = 1;
            break;
        case TEXTURE_FACE_POSITIVE_Z:
            slice = 2;
            break;
        case TEXTURE_FACE_NEGATIVE_Z:
            slice = 3;
            break;
        case TEXTURE_FACE_POSITIVE_Y:
            slice = 4;
            break;
        case TEXTURE_FACE_NEGATIVE_Y:
            slice = 5;
            break;
        }

        [self->uid getBytes:self->mappedData bytesPerRow:stride bytesPerImage:sz fromRegion:rgn mipmapLevel:level slice:slice];
        self->mappedSlice = slice;
    }
    else
    {
        [self->uid getBytes:self->mappedData bytesPerRow:stride fromRegion:rgn mipmapLevel:level];
    }

    if (self->format == TEXTURE_FORMAT_R4G4B4A4)
    {
        _FlipRGBA4_ABGR4(self->mappedData, self->mappedData, sz);
    }
    else if (self->format == TEXTURE_FORMAT_R5G5B5A1)
    {
        _ABGR1555toRGBA5551(self->mappedData, self->mappedData, sz);
    }
    else if ((self->format == TEXTURE_FORMAT_R8G8B8A8) && self->CreationDesc().isRenderTarget)
    {
        _SwapRB8(self->mappedData, self->mappedData, sz);
    }

    self->is_mapped = true;
    self->mappedLevel = level;

    return self->mappedData;
}

//------------------------------------------------------------------------------

static void
metal_Texture_Unmap(Handle tex)
{
    TextureMetal_t* self = TextureMetalPool::Get(tex);
    MTLRegion rgn;
    uint32 stride = TextureStride(self->format, Size2i([self->uid width], [self->uid height]), self->mappedLevel);
    Size2i ext = TextureExtents(Size2i([self->uid width], [self->uid height]), self->mappedLevel);
    unsigned sz = TextureSize(self->format, [self->uid width], [self->uid height], self->mappedLevel);

    DVASSERT(self->is_mapped);

    rgn.origin.x = 0;
    rgn.origin.y = 0;
    rgn.origin.z = 0;
    rgn.size.width = ext.dx;
    rgn.size.height = ext.dy;
    rgn.size.depth = 1;

    if (self->format == TEXTURE_FORMAT_R4G4B4A4)
    {
        _FlipRGBA4_ABGR4(self->mappedData, self->mappedData, sz);
    }
    else if (self->format == TEXTURE_FORMAT_R5G5B5A1)
    {
        _RGBA5551toABGR1555(self->mappedData, self->mappedData, sz);
    }

    if (self->is_cubemap)
    {
        [self->uid replaceRegion:rgn mipmapLevel:self->mappedLevel slice:self->mappedSlice withBytes:self->mappedData bytesPerRow:stride bytesPerImage:sz];
    }
    else
    {
        [self->uid replaceRegion:rgn mipmapLevel:self->mappedLevel withBytes:self->mappedData bytesPerRow:stride];
    }
    #if RHI_METAL__USE_PURGABLE_STATE
    [self->uid setPurgeableState:MTLPurgeableStateNonVolatile];
    #endif

    self->is_mapped = false;
    ::free(self->mappedData);
    self->mappedData = nullptr;
    self->MarkRestored();
}

//------------------------------------------------------------------------------

void metal_Texture_Update(Handle tex, const void* data, uint32 level, TextureFace face)
{
    TextureMetal_t* self = TextureMetalPool::Get(tex);
    Size2i ext = TextureExtents(Size2i(self->width, self->height), level);
    uint32 sz = TextureSize(self->format, self->width, self->height, level);
    uint32 stride = TextureStride(self->format, Size2i(self->width, self->height), level);
    MTLRegion rgn;
    NSUInteger slice = 0;

    switch (face)
    {
    case TEXTURE_FACE_NEGATIVE_X:
        slice = 0;
        break;
    case TEXTURE_FACE_POSITIVE_X:
        slice = 1;
        break;
    case TEXTURE_FACE_POSITIVE_Z:
        slice = 2;
        break;
    case TEXTURE_FACE_NEGATIVE_Z:
        slice = 3;
        break;
    case TEXTURE_FACE_POSITIVE_Y:
        slice = 4;
        break;
    case TEXTURE_FACE_NEGATIVE_Y:
        slice = 5;
        break;
    }

    if (self->format == TEXTURE_FORMAT_R4G4B4A4 || self->format == TEXTURE_FORMAT_R5G5B5A1)
    {
        metal_Texture_Map(tex, level, face);
        memcpy(self->mappedData, data, sz);
        metal_Texture_Unmap(tex);
    }
    else
    {
        rgn.origin.x = 0;
        rgn.origin.y = 0;
        rgn.origin.z = 0;
        rgn.size.width = ext.dx;
        rgn.size.height = ext.dy;
        rgn.size.depth = 1;

        if (self->is_cubemap)
        {
            [self->uid replaceRegion:rgn mipmapLevel:level slice:slice withBytes:data bytesPerRow:stride bytesPerImage:sz];
        }
        else
        {
            [self->uid replaceRegion:rgn mipmapLevel:level withBytes:data bytesPerRow:stride];
        }
    }

    #if RHI_METAL__USE_PURGABLE_STATE
    [self->uid setPurgeableState:MTLPurgeableStateNonVolatile];
    #endif
    self->MarkRestored();
}

//------------------------------------------------------------------------------

static bool
metal_Texture_NeedRestore(Handle tex)
{
    TextureMetal_t* self = TextureMetalPool::Get(tex);

    return self->NeedRestore();
}

//------------------------------------------------------------------------------

namespace TextureMetal
{
void Init(uint32 maxCount)
{
    TextureMetalPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_Texture_Create = &metal_Texture_Create;
    dispatch->impl_Texture_Delete = &metal_Texture_Delete;
    dispatch->impl_Texture_Map = &metal_Texture_Map;
    dispatch->impl_Texture_Unmap = &metal_Texture_Unmap;
    dispatch->impl_Texture_Update = &metal_Texture_Update;
    dispatch->impl_Texture_NeedRestore = &metal_Texture_NeedRestore;
}

void SetToRHIFragment(Handle tex, unsigned unitIndex, id<MTLRenderCommandEncoder> ce)
{
    TextureMetal_t* self = TextureMetalPool::Get(tex);

    [ce setFragmentTexture:self->uid atIndex:unitIndex];

    //_CheckAllTextures();
    /*
#if RHI_METAL__USE_PURGABLE_STATE
    if (self->need_restoring)
    {
        MTLPurgeableState s = [self->uid setPurgeableState:MTLPurgeableStateKeepCurrent];

        DVASSERT(s != MTLPurgeableStateKeepCurrent);
        if (s == MTLPurgeableStateEmpty)
        {
            if (!self->NeedRestore())
            {
                self->MarkNeedRestore();
                DAVA::Logger::Info("tex-lost  %ux%u  ps= %i", self->width, self->height, int(s));
            }
        }
        //-        [self->uid setPurgeableState:s];
    }
#endif
*/
    /*
    if( self->need_restoring && !self->NeedRestore() )
    {
        MTLPurgeableState s = [self->uid setPurgeableState:MTLPurgeableStateKeepCurrent];

        if (s == MTLPurgeableStateEmpty)
        {
            Texture::Descriptor desc = self->creationDesc;

            _Destroy( self );
            memset( desc.initialData, 0, sizeof(desc.initialData) );
            _Construct( self, desc );

            self->MarkNeedRestore();
            DAVA::Logger::Info("tex-%u lost  (%ux%u)", RHI_HANDLE_INDEX(tex), self->width, self->height );
        }
    }

    if (!self->NeedRestore())
        [ce setFragmentTexture:self->uid atIndex:unitIndex];    
*/
}

void SetToRHIVertex(Handle tex, unsigned unitIndex, id<MTLRenderCommandEncoder> ce)
{
    TextureMetal_t* self = TextureMetalPool::Get(tex);

    [ce setVertexTexture:self->uid atIndex:unitIndex];
}

void SetAsRenderTarget(Handle tex, MTLRenderPassDescriptor* desc, unsigned target_i)
{
    TextureMetal_t* self = TextureMetalPool::Get(tex);
    DVASSERT(!self->is_cubemap);

    DVASSERT(self->uid);
    desc.colorAttachments[target_i].texture = self->uid;
}

void SetAsResolveRenderTarget(Handle tex, MTLRenderPassDescriptor* desc)
{
    TextureMetal_t* self = TextureMetalPool::Get(tex);
    DVASSERT(!self->is_cubemap);

    DVASSERT(self->uid);
    desc.colorAttachments[0].resolveTexture = self->uid;
}

void SetAsDepthStencil(Handle tex, MTLRenderPassDescriptor* desc)
{
    TextureMetal_t* self = TextureMetalPool::Get(tex);

    desc.depthAttachment.texture = self->uid;
    desc.stencilAttachment.texture = self->uid2;
}

void SetAsResolveDepthStencil(Handle tex, MTLRenderPassDescriptor* desc)
{
    TextureMetal_t* self = TextureMetalPool::Get(tex);

    desc.depthAttachment.resolveTexture = self->uid;
    desc.stencilAttachment.resolveTexture = self->uid2;
}

unsigned
NeedRestoreCount()
{
    return TextureMetalPool::PendingRestoreCount();
}

void
MarkAllNeedRestore()
{
    for (TextureMetalPool::Iterator t = TextureMetalPool::Begin(), t_end = TextureMetalPool::End(); t != t_end; ++t)
    {
        if (t->need_restoring)
            t->MarkNeedRestore();
    }
}

void
ReCreateAll()
{
    for (TextureMetalPool::Iterator t = TextureMetalPool::Begin(), t_end = TextureMetalPool::End(); t != t_end; ++t)
    {
        TextureMetal_t* self = &(*t);
        Texture::Descriptor desc = t->creationDesc;

        _Destroy(self);
        memset(desc.initialData, 0, sizeof(desc.initialData));
        _Construct(self, desc);

        if (self->need_restoring)
            self->MarkNeedRestore();
    }
}

} // namespace TextureMetal

//==============================================================================
} // namespace rhi

#endif //#if !(TARGET_IPHONE_SIMULATOR==1)
