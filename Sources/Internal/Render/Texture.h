#ifndef __DAVAENGINE_TEXTURE_H__
#define __DAVAENGINE_TEXTURE_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/BaseObject.h"
#include "Base/FastName.h"
#include "FileSystem/FilePath.h"
#include "Concurrency/Mutex.h"
#include "Render/RHI/rhi_Public.h"
#include "Render/RenderBase.h"
#include "Render/UniqueStateSet.h"

#include "MemoryManager/MemoryProfiler.h"

namespace DAVA
{
/**
	\ingroup render
	\brief Class that represents texture objects in our SDK.
	This class support the following formats: RGBA8888, RGB565, RGBA4444, A8 on all platforms.
	For iOS it also support compressed PVR formats. (PVR2 and PVR4)
 */
class Image;
class TextureDescriptor;
class File;
class Texture;

#ifdef USE_FILEPATH_IN_MAP
using TexturesMap = Map<FilePath, Texture*>;
#else //#ifdef USE_FILEPATH_IN_MAP
using TexturesMap = Map<String, Texture*>;
#endif //#ifdef USE_FILEPATH_IN_MAP

class Texture : public BaseObject
{
    DAVA_ENABLE_CLASS_ALLOCATION_TRACKING(ALLOC_POOL_TEXTURE)
public:
    enum TextureState : uint8
    {
        STATE_INVALID = 0,
        STATE_DATA_LOADED,
        STATE_VALID
    };

    const static uint32 MINIMAL_WIDTH = 8;
    const static uint32 MINIMAL_HEIGHT = 8;

    const static uint32 INVALID_CUBEMAP_FACE = -1;
    const static uint32 CUBE_FACE_COUNT = 6;

    static Array<String, CUBE_FACE_COUNT> FACE_NAME_SUFFIX;

    struct
    FBODescriptor
    {
        uint32 width = 0;
        uint32 height = 0;
        uint32 sampleCount = 1;
        PixelFormat format = PixelFormat::FORMAT_INVALID;
        rhi::TextureType textureType = rhi::TEXTURE_TYPE_2D;
        bool needDepth = false;
        bool needPixelReadback = false;
        bool ensurePowerOf2 = true;
    };

    // Main constructors
    /**
        \brief Create texture from data arrray
        This function creates texture from given format, data pointer and width + height

        \param[in] format desired pixel format
        \param[in] data desired data
        \param[in] width width of new texture
        \param[in] height height of new texture
        \param[in] generateMipMaps generate mipmaps or not
     */
    static Texture* CreateFromData(PixelFormat format, const uint8* data, uint32 width, uint32 height, bool generateMipMaps);

    /**
        \brief Create texture from data arrray stored at Image
        This function creates texture from given image

        \param[in] image stores data
        \param[in] generateMipMaps generate mipmaps or not
     */
    static Texture* CreateFromData(Image* img, bool generateMipMaps);

    static Texture* CreateFromData(const Vector<Image*>& images);

    /**
        \brief Create text texture from data arrray
        This function creates texture from given format, data pointer and width + height, but adds addInfo string to relativePathname variable for easy identification of textures

        \param[in] format desired pixel format
        \param[in] data desired data
        \param[in] width width of new texture
        \param[in] height height of new texture
        \param[in] addInfo additional info
     */
    static Texture* CreateTextFromData(PixelFormat format, uint8* data, uint32 width, uint32 height, bool generateMipMaps, const char* addInfo = 0);

    /**
        \brief Create texture from given file. Supported formats .png, .pvr (only on iOS).
		If file cannot be opened, returns "pink placeholder" texture.
        \param[in] pathName path to the png or pvr file
     */
    static Texture* CreateFromFile(const FilePath& pathName, const FastName& group = FastName(), rhi::TextureType typeHint = rhi::TEXTURE_TYPE_2D);

    /**
        \brief Create texture from given file. Supported formats .png, .pvr (only on iOS).
		If file cannot be opened, returns 0
        \param[in] pathName path to the png or pvr file
     */
    static Texture* PureCreate(const FilePath& pathName, const FastName& group = FastName());

    static Texture* CreatePink(rhi::TextureType requestedType = rhi::TEXTURE_TYPE_2D, bool checkers = true);

    static Texture* CreateFBO(uint32 width, uint32 height, PixelFormat format, bool needDepth = false,
                              rhi::TextureType requestedType = rhi::TEXTURE_TYPE_2D, bool ensurePowerOf2 = true);

    static Texture* CreateFBO(const FBODescriptor& desc);

    /**
        \brief Get texture from cache.
        If texture isn't in cache, returns 0
        \param[in] name path of TextureDescriptor
     */
    static Texture* Get(const FilePath& name);

    int32 Release() override;

    static void DumpTextures();

    inline int32 GetWidth() const
    {
        return width;
    }
    inline int32 GetHeight() const
    {
        return height;
    }

    void GenerateMipmaps();

    void TexImage(int32 level, uint32 width, uint32 height, const void* _data, uint32 dataSize, uint32 cubeFaceId);

    void SetWrapMode(rhi::TextureAddrMode wrapU, rhi::TextureAddrMode wrapV, rhi::TextureAddrMode wrapW = rhi::TEXADDR_WRAP);
    void SetMinMagFilter(rhi::TextureFilter minFilter, rhi::TextureFilter magFilter, rhi::TextureMipFilter mipFilter);

    /**
        \brief Function to receive pathname of texture object
        \returns pathname of texture
     */
    const FilePath& GetPathname() const;
    void SetPathname(const FilePath& path);

    Image* CreateImageFromMemory();

    bool IsPinkPlaceholder();

    void Reload();
    void ReloadAs(eGPUFamily gpuFamily);
    void ReloadFromData(PixelFormat format, uint8* data, uint32 width, uint32 height);

    inline TextureState GetState() const;

    void SetDebugInfo(const String& _debugInfo);

    static const TexturesMap& GetTextureMap();

    uint32 GetDataSize() const;

    static void SetGPULoadingOrder(const Vector<eGPUFamily>& gpuLoadingOrder);
    static const Vector<eGPUFamily>& GetGPULoadingOrder();
    static eGPUFamily GetPrimaryGPUForLoading();

    inline eGPUFamily GetSourceFileGPUFamily() const;
    inline TextureDescriptor* GetDescriptor() const;

    PixelFormat GetFormat() const;

    static void SetPixelization(bool value);

    uint32 GetBaseMipMap() const;

    static rhi::HSamplerState CreateSamplerStateHandle(const rhi::SamplerState::Descriptor::Sampler& samplerState);

    static eGPUFamily GetGPUForLoading(const eGPUFamily requestedGPU, const TextureDescriptor* descriptor);

protected:
    void RestoreRenderResource();

    void ReleaseTextureData();

    static void AddToMap(Texture* tex);

    static Texture* CreateFromImage(TextureDescriptor* descriptor, eGPUFamily gpu);

    bool LoadImages(eGPUFamily gpu, Vector<Image*>* images);

    void SetParamsFromImages(const Vector<Image*>* images);

    void FlushDataToRenderer(Vector<Image*>* images);

    void ReleaseImages(Vector<Image*>* images);

    void MakePink(bool checkers = true);

    Texture();
    virtual ~Texture();

    bool IsLoadAvailable(const eGPUFamily gpuFamily) const;

public: // properties for fast access
    rhi::HTexture handle;
    rhi::HTexture handleDepthStencil; //it's legacy and should be removed. (maybe together with CreateFBO method)
    rhi::HSamplerState samplerStateHandle;
    rhi::HTextureSet singleTextureSet;
    rhi::SamplerState::Descriptor::Sampler samplerState;

    uint32 width : 16; // texture width
    uint32 height : 16; // texture height

    eGPUFamily loadedAsFile;

    TextureState state : 2;
    uint32 textureType : 2;

    bool isRenderTarget : 1;
    bool isPink : 1;

    FastName debugInfo;

    TextureDescriptor* texDescriptor;

    static Mutex textureMapMutex;

    static TexturesMap textureMap;
    static Vector<eGPUFamily> gpuLoadingOrder;

    static bool pixelizationFlag;
};

// Implementation of inline functions

inline eGPUFamily Texture::GetSourceFileGPUFamily() const
{
    return loadedAsFile;
}

inline Texture::TextureState Texture::GetState() const
{
    return state;
}

inline TextureDescriptor* Texture::GetDescriptor() const
{
    return texDescriptor;
}
};

#endif // __DAVAENGINE_TEXTUREGLES_H__
