#include "Render/RenderBase.h"
#include "Render/Texture.h"
#include "Utils/Utils.h"
#include "Logger/Logger.h"
#include "Debug/DVAssert.h"
#include "Utils/Utils.h"
#include "Render/Renderer.h"
#include "Utils/StringFormat.h"
#include "Time/SystemTimer.h"
#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Render/RenderHelper.h"

#if defined(__DAVAENGINE_IPHONE__)
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#elif defined(__DAVAENGINE_MACOS__)
#include <ApplicationServices/ApplicationServices.h>
#endif //PLATFORMS

#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"
#include "Render/Image/ImageConvert.h"

#include "Render/TextureDescriptor.h"
#include "Render/GPUFamilyDescriptor.h"
#include "Math/MathHelpers.h"
#include "Concurrency/LockGuard.h"

#define DAVA_DEBUG_TEXTURE_DISABLE_LOADING 0

namespace DAVA
{

#if (DAVA_DEBUG_TEXTURE_DISABLE_LOADING)
Texture* GetSharedPinkTexture()
{
    static Texture* pink = nullptr;
    if (pink == nullptr)
    {
        pink = Texture::CreatePink();
    }
    return SafeRetain(pink);
}
#endif

namespace Validator
{
bool IsFormatHardwareSupported(PixelFormat format)
{
    const auto& formatDescriptor = PixelFormatDescriptor::GetPixelFormatDescriptor(format);
    return formatDescriptor.isHardwareSupported;
}

bool AreImagesSquare(const Vector<DAVA::Image*>& imageSet)
{
    for (Image* image : imageSet)
    {
        if (!IsPowerOf2(image->GetWidth()) || !IsPowerOf2(image->GetHeight()))
        {
            return false;
        }
    }

    return true;
}

bool AreImagesCorrectForTexture(const Vector<DAVA::Image*>& imageSet)
{
    if (0 == imageSet.size())
    {
        return false;
    }

    if (imageSet[0]->width < Texture::MINIMAL_WIDTH || imageSet[0]->height < Texture::MINIMAL_HEIGHT)
    {
        Logger::Error("[TextureValidator] Loaded images size is too small. Minimal size for texture is 8x8");
        return false;
    }

    bool isSizeCorrect = Validator::AreImagesSquare(imageSet);
    if (!isSizeCorrect)
    {
        Logger::Error("[TextureValidator] Size if loaded images is invalid (not power of 2)");
        return false;
    }

    return true;
}

bool CheckAndFixImageFormat(Vector<Image*>* images)
{
    Vector<Image*>& imageSet = *images;
    PixelFormat format = imageSet[0]->format;
    if (IsFormatHardwareSupported(format))
    {
        return true;
    }

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)
    //we should decode all images for RE/QE
    if (ImageConvert::CanConvertFromTo(format, FORMAT_RGBA8888))
#else
    //We should decode only RGB888 into RGBA8888
    if (format == PixelFormat::FORMAT_RGB888 && ImageConvert::CanConvertFromTo(format, FORMAT_RGBA8888))
#endif
    {
        for (Image*& image : imageSet)
        {
            Image* newImage = Image::Create(image->width, image->height, FORMAT_RGBA8888);
            bool converted = ImageConvert::ConvertImage(image, newImage);
            if (converted)
            {
                newImage->mipmapLevel = image->mipmapLevel;
                newImage->cubeFaceID = image->cubeFaceID;

                image->Release();
                image = newImage;
            }
            else
            {
                SafeRelease(newImage);
                return false;
            }
        }

        return true;
    }

    return false;
}
}

Array<String, Texture::CUBE_FACE_COUNT> Texture::FACE_NAME_SUFFIX =
{ {
String("_px"),
String("_nx"),
String("_py"),
String("_ny"),
String("_pz"),
String("_nz")
} };

class TextureMemoryUsageInfo
{
public:
    TextureMemoryUsageInfo()
    {
        pvrTexturesMemoryUsed = 0;
        texturesMemoryUsed = 0;
        fboMemoryUsed = 0;
    }

    void AllocPVRTexture(int size)
    {
        pvrTexturesMemoryUsed += size;
    }

    void ReleasePVRTexture(int size)
    {
        pvrTexturesMemoryUsed -= size;
    }

    void AllocTexture(int size)
    {
        texturesMemoryUsed += size;
    }

    void ReleaseTexture(int size)
    {
        texturesMemoryUsed -= size;
    }

    void AllocFBOTexture(int size)
    {
        fboMemoryUsed += size;
    }

    void ReleaseFBOTexture(int size)
    {
        fboMemoryUsed -= size;
    }

    // STATISTICS
    int pvrTexturesMemoryUsed;
    int texturesMemoryUsed;
    int fboMemoryUsed;
};

static TextureMemoryUsageInfo texMemoryUsageInfo;

TexturesMap Texture::textureMap;
Vector<eGPUFamily> Texture::gpuLoadingOrder;

Mutex Texture::textureMapMutex;

static int32 textureFboCounter = 0;

bool Texture::pixelizationFlag = false;

// Main constructors
Texture* Texture::Get(const FilePath& pathName)
{
    LockGuard<Mutex> guard(textureMapMutex);

    Texture* texture = nullptr;
    TexturesMap::iterator it = textureMap.find(FILEPATH_MAP_KEY(pathName));
    if (it != textureMap.end())
    {
        texture = it->second;
        texture->Retain();
    }
    return texture;
}

void Texture::AddToMap(Texture* tex)
{
    if (!tex->texDescriptor->pathname.IsEmpty())
    {
        DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

        LockGuard<Mutex> guard(textureMapMutex);
        DVASSERT(textureMap.find(FILEPATH_MAP_KEY(tex->texDescriptor->pathname)) == textureMap.end());
        textureMap[FILEPATH_MAP_KEY(tex->texDescriptor->pathname)] = tex;
    }
}

Texture::Texture()
    : handle(rhi::InvalidHandle)
    , samplerStateHandle(rhi::InvalidHandle)
    , singleTextureSet(rhi::InvalidHandle)
    , width(0)
    , height(0)
    , loadedAsFile(GPU_ORIGIN)
    , state(STATE_INVALID)
    , textureType(rhi::TEXTURE_TYPE_2D)
    , isRenderTarget(false)
    , isPink(false)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    texDescriptor = new TextureDescriptor;
    Renderer::GetSignals().needRestoreResources.Connect(this, &Texture::RestoreRenderResource);
}

Texture::~Texture()
{
    Renderer::GetSignals().needRestoreResources.Disconnect(this);
    ReleaseTextureData();
    SafeDelete(texDescriptor);
}

void Texture::ReleaseTextureData()
{
    if (handle.IsValid())
    {
        rhi::DeleteTexture(handle);
        handle = rhi::HTexture(rhi::InvalidHandle);
    }

    if (handleDepthStencil.IsValid())
    {
        rhi::DeleteTexture(handleDepthStencil);
        handleDepthStencil = rhi::HTexture();
    }

    if (samplerStateHandle.IsValid())
    {
        rhi::ReleaseSamplerState(samplerStateHandle);
        samplerStateHandle = rhi::HSamplerState(rhi::InvalidHandle);
    }

    if (singleTextureSet.IsValid())
    {
        rhi::ReleaseTextureSet(singleTextureSet);
        singleTextureSet = rhi::HTextureSet(rhi::InvalidHandle);
    }

    state = STATE_INVALID;
    isRenderTarget = false;
}

Texture* Texture::CreateTextFromData(PixelFormat format, uint8* data, uint32 width, uint32 height, bool generateMipMaps, const char* addInfo)
{
#if (DAVA_DEBUG_TEXTURE_DISABLE_LOADING)
    return GetSharedPinkTexture();
#endif

    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    Texture* tx = CreateFromData(format, data, width, height, generateMipMaps);

    if (!addInfo)
    {
        tx->texDescriptor->pathname = Format("Text texture %d", textureFboCounter);
    }
    else
    {
        tx->texDescriptor->pathname = Format("Text texture %d info:%s", textureFboCounter, addInfo);
    }
    AddToMap(tx);

    textureFboCounter++;
    return tx;
}

void Texture::TexImage(int32 level, uint32 width, uint32 height, const void* _data, uint32 dataSize, uint32 cubeFaceId)
{
    rhi::UpdateTexture(handle, _data, level, rhi::TextureFace(cubeFaceId));
}

Texture* Texture::CreateFromData(PixelFormat _format, const uint8* _data, uint32 _width, uint32 _height, bool generateMipMaps)
{
#if (DAVA_DEBUG_TEXTURE_DISABLE_LOADING)
    return GetSharedPinkTexture();
#endif

    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    if ((_width < Texture::MINIMAL_WIDTH || _height < Texture::MINIMAL_HEIGHT) && (_format == FORMAT_PVR2 || _format == FORMAT_PVR4))
    {
        return nullptr;
    }

    Image* image = Image::CreateFromData(_width, _height, _format, _data);
    if (nullptr == image)
        return nullptr;

    Texture* texture = new Texture();
    texture->texDescriptor->Initialize(rhi::TEXADDR_CLAMP, generateMipMaps);

    Vector<Image*>* images = new Vector<Image*>();
    images->push_back(image);

    Validator::CheckAndFixImageFormat(images);

    texture->SetParamsFromImages(images);
    texture->FlushDataToRenderer(images);

    return texture;
}

Texture* Texture::CreateFromData(Image* image, bool generateMipMaps)
{
#if (DAVA_DEBUG_TEXTURE_DISABLE_LOADING)
    return GetSharedPinkTexture();
#endif

    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    if ((image->width < Texture::MINIMAL_WIDTH || image->height < Texture::MINIMAL_HEIGHT) && (image->format == FORMAT_PVR2 || image->format == FORMAT_PVR4))
    {
        return nullptr;
    }

    Texture* texture = new Texture();
    texture->texDescriptor->Initialize(rhi::TEXADDR_CLAMP, generateMipMaps);

    Vector<Image*>* images = new Vector<Image*>();
    image->Retain();
    images->push_back(image);

    Validator::CheckAndFixImageFormat(images);

    texture->SetParamsFromImages(images);
    texture->FlushDataToRenderer(images);

    return texture;
}

Texture* Texture::CreateFromData(const Vector<Image*>& imgs)
{
#if (DAVA_DEBUG_TEXTURE_DISABLE_LOADING)
    return GetSharedPinkTexture();
#endif

    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    Texture* texture = new Texture();
    texture->texDescriptor->Initialize(rhi::TEXADDR_CLAMP, false);

    Vector<Image*>* images = new Vector<Image*>(imgs);
    for (Image* image : (*images))
        image->Retain();

    Validator::CheckAndFixImageFormat(images);

    texture->SetParamsFromImages(images);
    texture->FlushDataToRenderer(images);

    return texture;
}

void Texture::SetWrapMode(rhi::TextureAddrMode wrapU, rhi::TextureAddrMode wrapV, rhi::TextureAddrMode wrapW)
{
    samplerState.addrU = wrapU;
    samplerState.addrV = wrapV;
    samplerState.addrW = wrapW;

    rhi::ReleaseSamplerState(samplerStateHandle);
    samplerStateHandle = CreateSamplerStateHandle(samplerState);
}

void Texture::SetMinMagFilter(rhi::TextureFilter minFilter, rhi::TextureFilter magFilter, rhi::TextureMipFilter mipFilter)
{
    samplerState.minFilter = minFilter;
    samplerState.magFilter = magFilter;
    samplerState.mipFilter = mipFilter;

    rhi::ReleaseSamplerState(samplerStateHandle);
    samplerStateHandle = CreateSamplerStateHandle(samplerState);
}

void Texture::GenerateMipmaps()
{
    DVASSERT(0, "Mipmap generation on fly is not supported anymore!");
}

Texture* Texture::CreateFromImage(TextureDescriptor* descriptor, eGPUFamily gpu)
{
#if (DAVA_DEBUG_TEXTURE_DISABLE_LOADING)
    return GetSharedPinkTexture();
#endif

    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    Texture* texture = new Texture();
    texture->texDescriptor->Initialize(descriptor);

    Vector<Image*>* images = new Vector<Image*>();

    bool loaded = texture->LoadImages(gpu, images);
    if (!loaded)
    {
        SafeDelete(images);
        SafeRelease(texture);
        return nullptr;
    }

    texture->SetParamsFromImages(images);
    texture->FlushDataToRenderer(images);

    if (!texture->singleTextureSet.IsValid())
    {
        Logger::Error
        (
        "[Texture::CreateFromImage] Cannot create rhi.texture from image. Descriptor: %s, GPU: %s",
        descriptor->pathname.GetAbsolutePathname().c_str(), GlobalEnumMap<eGPUFamily>::Instance()->ToString(gpu)
        );
        SafeRelease(texture);
        return nullptr;
    }

    return texture;
}

bool Texture::LoadImages(eGPUFamily gpu, Vector<Image*>* images)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    DVASSERT(gpu != GPU_INVALID);

    if (!IsLoadAvailable(gpu))
    {
        Logger::Error("[Texture::LoadImages] Load not available: invalid requested GPU family (%s)", GlobalEnumMap<eGPUFamily>::Instance()->ToString(gpu));
        return false;
    }

    uint32 baseMipMap = GetBaseMipMap();
    ImageSystem::LoadingParams params;
    params.baseMipmap = baseMipMap;
    params.firstMipmapIndex = 0;
    params.minimalWidth = Texture::MINIMAL_WIDTH;
    params.minimalHeight = Texture::MINIMAL_HEIGHT;

    if (texDescriptor->IsCubeMap() && (!GPUFamilyDescriptor::IsGPUForDevice(gpu)))
    {
        Vector<FilePath> facePathes;
        texDescriptor->GetFacePathnames(facePathes);

        PixelFormat imagesFormat = FORMAT_INVALID;
        for (uint32 i = 0; i < CUBE_FACE_COUNT; ++i)
        {
            const FilePath& currentfacePath = facePathes[i];
            if (currentfacePath.IsEmpty())
                continue;

            Vector<Image*> faceImage;
            ImageSystem::Load(currentfacePath, faceImage, params);
            if (faceImage.empty())
            {
                Logger::Error("[Texture::LoadImages] Cannot open file %s", currentfacePath.GetAbsolutePathname().c_str());

                ReleaseImages(images);
                return false;
            }

            DVASSERT(faceImage.size() == 1);

            faceImage[0]->cubeFaceID = i;
            faceImage[0]->mipmapLevel = 0;

            //cubemap formats validation
            if (FORMAT_INVALID == imagesFormat)
            {
                imagesFormat = faceImage[0]->format;
            }
            else if (imagesFormat != faceImage[0]->format)
            {
                Logger::Error("[Texture::LoadImages] Face(%s) has different pixel format(%s)", currentfacePath.GetAbsolutePathname().c_str(), PixelFormatDescriptor::GetPixelFormatString(faceImage[0]->format));

                ReleaseImages(images);
                return false;
            }
            //end of cubemap formats validation

            if (texDescriptor->GetGenerateMipMaps())
            {
                Vector<Image*> mipmapsImages = faceImage[0]->CreateMipMapsImages();
                images->insert(images->end(), mipmapsImages.begin(), mipmapsImages.end());
                SafeRelease(faceImage[0]);
            }
            else
            {
                images->push_back(faceImage[0]);
            }
        }
    }
    else
    {
        Vector<FilePath> singleMipFiles;
        bool hasSingleMipFiles = texDescriptor->CreateSingleMipPathnamesForGPU(gpu, singleMipFiles);
        if (hasSingleMipFiles)
        {
            uint32 singleMipFilesCount = static_cast<uint32>(singleMipFiles.size());
            for (uint32 index = baseMipMap; index < singleMipFilesCount; ++index)
            {
                params.baseMipmap = 0;
                eErrorCode loadingCode = ImageSystem::Load(singleMipFiles[index], *images, params);
                if (loadingCode == eErrorCode::SUCCESS)
                {
                    ++params.firstMipmapIndex;
                }
            }

            params.baseMipmap = Max(static_cast<int32>(baseMipMap) - static_cast<int32>(singleMipFilesCount), 0);
        }

        FilePath multipleMipPathname = texDescriptor->CreateMultiMipPathnameForGPU(gpu);
        ImageSystem::Load(multipleMipPathname, *images, params);

        ImageSystem::EnsurePowerOf2Images(*images);
    }

    if (!Validator::AreImagesCorrectForTexture(*images))
    {
        ReleaseImages(images);
        return false;
    }

    if (!Validator::CheckAndFixImageFormat(images))
    {
        Logger::Error("[Texture::LoadImages] cannot create texture from images because of wrong image format");

        ReleaseImages(images);
        return false;
    }

    if (images->size() == 1 && texDescriptor->GetGenerateMipMaps())
    {
        Image* img = *images->begin();
        *images = img->CreateMipMapsImages(texDescriptor->dataSettings.GetIsNormalMap());
        SafeRelease(img);

        if (images->empty())
        {
            Logger::Error("[Texture::LoadImages] Can't create mipmaps for GPU (%s) for %s", GlobalEnumMap<eGPUFamily>::Instance()->ToString(gpu), texDescriptor->pathname.GetStringValue().c_str());
            return false;
        }
    }

    isPink = false;
    state = STATE_DATA_LOADED;

    return true;
}

void Texture::ReleaseImages(Vector<Image*>* images)
{
    for_each(images->begin(), images->end(), SafeRelease<Image>);
    images->clear();
}

void Texture::SetParamsFromImages(const Vector<Image*>* images)
{
    DVASSERT(images->size() != 0);

    Image* img = *images->begin();
    width = img->width;
    height = img->height;
    texDescriptor->format = img->format;

    textureType = (img->cubeFaceID != Texture::INVALID_CUBEMAP_FACE) ? rhi::TEXTURE_TYPE_CUBE : rhi::TEXTURE_TYPE_2D;

    state = STATE_DATA_LOADED;
}

void Texture::FlushDataToRenderer(Vector<Image*>* images)
{
    DVASSERT(images->size() != 0);

    const PixelFormatDescriptor& formatDescriptor = PixelFormatDescriptor::GetPixelFormatDescriptor(texDescriptor->format);
    rhi::Texture::Descriptor descriptor;
    descriptor.autoGenMipmaps = false;
    descriptor.isRenderTarget = false;
    descriptor.width = (*images)[0]->width;
    descriptor.height = (*images)[0]->height;
    descriptor.type = ((*images)[0]->cubeFaceID == Texture::INVALID_CUBEMAP_FACE) ? rhi::TEXTURE_TYPE_2D : rhi::TEXTURE_TYPE_CUBE;
    descriptor.format = formatDescriptor.format;
    descriptor.levelCount = static_cast<uint32>((descriptor.type == rhi::TEXTURE_TYPE_CUBE) ? images->size() / 6 : images->size());

    const uint32 oldLevelCountVerify = descriptor.levelCount; //to notify about wrong images
    for (Image* img : (*images))
    { // kostil for some wrong data
        descriptor.levelCount = Max(descriptor.levelCount, img->mipmapLevel + 1);
    }
    if (oldLevelCountVerify != descriptor.levelCount)
    {
        Logger::Error("Something wrong with image mipmap levels at %s", texDescriptor->pathname.GetStringValue().c_str());
    }

    DVASSERT(descriptor.format != static_cast<rhi::TextureFormat>(-1)); //unsupported format

#if 1

    if (descriptor.type == rhi::TEXTURE_TYPE_2D)
    {
        for (size_t i = 0, sz = images->size(); i < sz; ++i)
            descriptor.initialData[i] = (*images)[i]->data;
    }
    else if (descriptor.type == rhi::TEXTURE_TYPE_CUBE)
    {
        rhi::TextureFace face[] = { rhi::TEXTURE_FACE_POSITIVE_X, rhi::TEXTURE_FACE_NEGATIVE_X, rhi::TEXTURE_FACE_POSITIVE_Y, rhi::TEXTURE_FACE_NEGATIVE_Y, rhi::TEXTURE_FACE_POSITIVE_Z, rhi::TEXTURE_FACE_NEGATIVE_Z };
        void** data = descriptor.initialData;

        for (unsigned f = 0; f != CUBE_FACE_COUNT; ++f)
        {
            for (unsigned m = 0; m != descriptor.levelCount; ++m)
            {
                *data = nullptr;

                for (size_t i = 0, sz = images->size(); i < sz; ++i)
                {
                    Image* img = (*images)[i];

                    if (img->cubeFaceID == face[f] && img->mipmapLevel == m)
                    {
                        *data = img->data;
                        break;
                    }
                }

                ++data;
            }
        }
    }

    handle = rhi::CreateTexture(descriptor);
    if (handle != rhi::InvalidHandle)
    {
        rhi::TextureSetDescriptor textureSetDesc;
        textureSetDesc.fragmentTexture[0] = handle;
        textureSetDesc.fragmentTextureCount = 1;
        singleTextureSet = rhi::AcquireTextureSet(textureSetDesc);
    }
    else
    {
        singleTextureSet = rhi::HTextureSet(rhi::InvalidHandle);
    }

#else

    handle = rhi::CreateTexture(descriptor);
    DVASSERT(handle != rhi::InvalidHandle);

    rhi::TextureSetDescriptor textureSetDesc;
    textureSetDesc.fragmentTexture[0] = handle;
    textureSetDesc.fragmentTextureCount = 1;
    singleTextureSet = rhi::AcquireTextureSet(textureSetDesc);

    for (uint32 i = 0; i < (uint32)images->size(); ++i)
    {
        Image* img = (*images)[i];
        TexImage((img->mipmapLevel != (uint32)-1) ? img->mipmapLevel : i, img->width, img->height, img->data, img->dataSize, img->cubeFaceID);
    }

#endif

    samplerState.addrU = texDescriptor->drawSettings.wrapModeS;
    samplerState.addrV = texDescriptor->drawSettings.wrapModeT;
    samplerState.minFilter = pixelizationFlag ? rhi::TextureFilter::TEXFILTER_NEAREST : texDescriptor->drawSettings.minFilter;
    samplerState.magFilter = pixelizationFlag ? rhi::TextureFilter::TEXFILTER_NEAREST : texDescriptor->drawSettings.magFilter;
    samplerState.mipFilter = pixelizationFlag ? rhi::TextureMipFilter::TEXMIPFILTER_NONE : texDescriptor->drawSettings.mipFilter;

    rhi::ReleaseSamplerState(samplerStateHandle);
    samplerStateHandle = CreateSamplerStateHandle(samplerState);

    state = STATE_VALID;

    ReleaseImages(images);
    SafeDelete(images);
}

Texture* Texture::CreateFromFile(const FilePath& pathName, const FastName& group, rhi::TextureType typeHint)
{
#if (DAVA_DEBUG_TEXTURE_DISABLE_LOADING)
    return GetSharedPinkTexture();
#endif

    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    Texture* texture = PureCreate(pathName, group);
    if (nullptr == texture)
    {
        TextureDescriptor* descriptor = TextureDescriptor::CreateFromFile(pathName);
        if (descriptor)
        {
            texture = CreatePink(descriptor->IsCubeMap() ? rhi::TEXTURE_TYPE_CUBE : typeHint);
            texture->texDescriptor->Initialize(descriptor);
            SafeDelete(descriptor);
        }
        else
        {
            texture = CreatePink(typeHint);
            texture->texDescriptor->pathname = (!pathName.IsEmpty()) ? TextureDescriptor::GetDescriptorPathname(pathName) : FilePath();
        }

        texture->texDescriptor->SetQualityGroup(group);

        AddToMap(texture);
    }

    return texture;
}

Texture* Texture::PureCreate(const FilePath& pathName, const FastName& group)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    if (pathName.IsEmpty() || (pathName.GetType() == FilePath::PATH_IN_MEMORY))
        return nullptr;

    if (!Renderer::GetOptions()->IsOptionEnabled(RenderOptions::TEXTURE_LOAD_ENABLED))
        return nullptr;

    FilePath descriptorPathname = TextureDescriptor::GetDescriptorPathname(pathName);
    Texture* texture = Texture::Get(descriptorPathname);
    if (texture)
        return texture;

    TextureDescriptor* descriptor(TextureDescriptor::CreateFromFile(descriptorPathname));
    if (nullptr == descriptor)
        return nullptr;

    descriptor->SetQualityGroup(group);
    for (eGPUFamily gpu : gpuLoadingOrder)
    {
        eGPUFamily gpuForLoading = GetGPUForLoading(gpu, descriptor);
        texture = CreateFromImage(descriptor, gpuForLoading);
        if (texture)
        {
            texture->loadedAsFile = gpuForLoading;
            AddToMap(texture);
            break;
        }
    }

    if (texture == nullptr)
    {
        Logger::Error("[Texture::PureCreate] Cannot create texture. Descriptor: %s, GPU: %s",
                      descriptor->pathname.GetAbsolutePathname().c_str(), GlobalEnumMap<eGPUFamily>::Instance()->ToString(GetPrimaryGPUForLoading()));
    }

    delete descriptor;
    return texture;
}

void Texture::ReloadFromData(PixelFormat format, uint8* data, uint32 _width, uint32 _height)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    rhi::HTexture oldHandle = handle;
    ReleaseTextureData();

    Image* image = Image::CreateFromData(_width, _height, format, data);
    if (!image)
        return;

    Vector<Image*>* images = new Vector<Image*>();
    images->push_back(image);

    SetParamsFromImages(images);
    FlushDataToRenderer(images);
    rhi::ReplaceTextureInAllTextureSets(oldHandle, handle);
}

void Texture::Reload()
{
    ReloadAs(loadedAsFile);
}

void Texture::ReloadAs(eGPUFamily gpuFamily)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();
    rhi::HTexture oldHandle = handle;

    DVASSERT(isRenderTarget == false);

    ReleaseTextureData();

    bool descriptorReloaded = texDescriptor->Reload();

    eGPUFamily gpuForLoading = GetGPUForLoading(gpuFamily, texDescriptor);
    Vector<Image*>* images = new Vector<Image*>();

    bool loaded = false;
    if (descriptorReloaded && Renderer::GetOptions()->IsOptionEnabled(RenderOptions::TEXTURE_LOAD_ENABLED))
    {
        loaded = LoadImages(gpuForLoading, images);
    }

    if (loaded)
    {
        loadedAsFile = gpuForLoading;

        SetParamsFromImages(images);
        FlushDataToRenderer(images);
    }
    else
    {
        SafeDelete(images);

        Logger::Error("[Texture::ReloadAs] Cannot reload from file %s for GPU %s", texDescriptor->pathname.GetAbsolutePathname().c_str(), GlobalEnumMap<eGPUFamily>::Instance()->ToString(gpuFamily));
        MakePink();
    }
    rhi::ReplaceTextureInAllTextureSets(oldHandle, handle);
}

bool Texture::IsLoadAvailable(const eGPUFamily gpuFamily) const
{
    if (texDescriptor->IsCompressedFile())
    {
        return true;
    }

    if (GPUFamilyDescriptor::IsGPUForDevice(gpuFamily) && texDescriptor->compression[gpuFamily].format == FORMAT_INVALID)
    {
        return false;
    }

    return true;
}

int32 Texture::Release()
{
    if (GetRetainCount() == 1)
    {
        textureMapMutex.Lock();
        textureMap.erase(FILEPATH_MAP_KEY(texDescriptor->pathname));
        textureMapMutex.Unlock();
    }
    return BaseObject::Release();
}

Texture* Texture::CreateFBO(const Texture::FBODescriptor& fboDesc)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    uint32 w = fboDesc.width;
    uint32 h = fboDesc.height;
    PixelFormat format = fboDesc.format;
    bool needDepth = fboDesc.needDepth;
    rhi::TextureType requestedType = fboDesc.textureType;

    int32 dx = Max(static_cast<int32>(w), 8);
    if (fboDesc.ensurePowerOf2)
    {
        EnsurePowerOf2(dx);
    }

    int32 dy = Max(static_cast<int32>(h), 8);
    if (fboDesc.ensurePowerOf2)
    {
        EnsurePowerOf2(dy);
    }

    Texture* tx = new Texture();
    tx->width = dx;
    tx->height = dy;
    tx->textureType = requestedType;
    tx->texDescriptor->format = format;
    tx->samplerState.mipFilter = tx->texDescriptor->drawSettings.mipFilter = rhi::TEXMIPFILTER_NONE;
    tx->samplerStateHandle = CreateSamplerStateHandle(tx->samplerState);

    const PixelFormatDescriptor& formatDescriptor = PixelFormatDescriptor::GetPixelFormatDescriptor(format);
    rhi::Texture::Descriptor descriptor;
    descriptor.width = tx->width;
    descriptor.height = tx->height;
    descriptor.autoGenMipmaps = false;
    descriptor.isRenderTarget = true;
    descriptor.needRestore = false;
    descriptor.type = requestedType;
    descriptor.format = formatDescriptor.format;
    descriptor.sampleCount = fboDesc.sampleCount;
    if (fboDesc.needPixelReadback)
    {
        descriptor.cpuAccessRead = true;
        descriptor.cpuAccessWrite = false;
    }

    DVASSERT(descriptor.format != static_cast<rhi::TextureFormat>(-1)); //unsupported format
    tx->handle = rhi::CreateTexture(descriptor);

    if (needDepth)
    {
        descriptor.isRenderTarget = false;
        descriptor.format = rhi::TEXTURE_FORMAT_D24S8;
        descriptor.type = rhi::TEXTURE_TYPE_2D;
        tx->handleDepthStencil = rhi::CreateTexture(descriptor);
    }

    rhi::TextureSetDescriptor textureSetDesc;
    textureSetDesc.fragmentTexture[0] = tx->handle;
    textureSetDesc.fragmentTextureCount = 1;
    tx->singleTextureSet = rhi::AcquireTextureSet(textureSetDesc);

    tx->isRenderTarget = true;
    tx->texDescriptor->pathname = Format("FBO texture %d", textureFboCounter);
    AddToMap(tx);

    textureFboCounter++;

    return tx;
}

Texture* Texture::CreateFBO(uint32 w, uint32 h, PixelFormat format, bool needDepth, rhi::TextureType requestedType, bool ensurePowerOf2)
{
    FBODescriptor fboDesc;

    fboDesc.width = w;
    fboDesc.height = h;
    fboDesc.format = format;
    fboDesc.needDepth = needDepth;
    fboDesc.needPixelReadback = false;
    fboDesc.textureType = requestedType;
    fboDesc.ensurePowerOf2 = ensurePowerOf2;

    return CreateFBO(fboDesc);
}

void Texture::DumpTextures()
{
    uint32 allocSize = 0;
    int32 cnt = 0;
    Logger::FrameworkDebug("============================================================");
    Logger::FrameworkDebug("--------------- Currently allocated textures ---------------");

    textureMapMutex.Lock();
    for (TexturesMap::iterator it = textureMap.begin(); it != textureMap.end(); ++it)
    {
        Texture* t = it->second;
        Logger::FrameworkDebug("%s with id %d (%dx%d) retainCount: %d debug: %s format: %s", t->texDescriptor->pathname.GetAbsolutePathname().c_str(), static_cast<uint32>(t->handle), t->width, t->height,
                               t->GetRetainCount(), t->debugInfo.c_str(), PixelFormatDescriptor::GetPixelFormatString(t->texDescriptor->format));
        cnt++;

        DVASSERT((0 <= t->texDescriptor->format) && (t->texDescriptor->format < FORMAT_COUNT));
        if (FORMAT_INVALID != t->texDescriptor->format)
        {
            allocSize += t->width * t->height * PixelFormatDescriptor::GetPixelFormatSizeInBits(t->texDescriptor->format);
        }
    }
    textureMapMutex.Unlock();

    Logger::FrameworkDebug("      Total allocated textures %d    memory size %d", cnt, allocSize / 8);
    Logger::FrameworkDebug("============================================================");
}

void Texture::SetDebugInfo(const String& _debugInfo)
{
#if defined(__DAVAENGINE_DEBUG__)
    debugInfo = FastName(_debugInfo.c_str());
#endif
}

void Texture::RestoreRenderResource()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();
    if ((!handle.IsValid()) || (!NeedRestoreTexture(handle)))
        return;

    Vector<Image*> images;

    const FilePath& relativePathname = texDescriptor->GetSourceTexturePathname();
    FilePath::ePathType pathType = relativePathname.GetType();

    bool shouldMakePink = isPink || (pathType == FilePath::PATH_EMPTY);

    if ((pathType == FilePath::PATH_IN_FILESYSTEM) || (pathType == FilePath::PATH_IN_RESOURCES) || (pathType == FilePath::PATH_IN_DOCUMENTS))
    {
        eGPUFamily gpuForLoading = GetGPUForLoading(loadedAsFile, texDescriptor);
        LoadImages(gpuForLoading, &images);
        if (images.empty())
        {
            String absolutePath = relativePathname.GetAbsolutePathname();
            Logger::Error("Unable to restore texture from file: %s", absolutePath.c_str());
            shouldMakePink = true;
        }
    }

    if (shouldMakePink)
    {
        DVASSERT(images.empty());

        if (texDescriptor->IsCubeMap())
        {
            for (uint32 i = 0; i < Texture::CUBE_FACE_COUNT; ++i)
            {
                Image* img = Image::Create(width, height, FORMAT_RGBA8888);
                img->MakePink(true);
                img->cubeFaceID = i;
                img->mipmapLevel = 0;
                images.push_back(img);
            }
        }
        else
        {
            Image* img = Image::Create(width, height, FORMAT_RGBA8888);
            img->MakePink(true);
            images.push_back(img);
        }
    }

    for (uint32 i = 0, sz = static_cast<uint32>(images.size()); i < sz; ++i)
    {
        Image* img = images[i];
        TexImage((img->mipmapLevel != static_cast<uint32>(-1)) ? img->mipmapLevel : i, img->width, img->height, img->data, img->dataSize, img->cubeFaceID);
    }

    ReleaseImages(&images);
}

Image* Texture::CreateImageFromMemory()
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    Image* image = nullptr;

    void* mappedData = rhi::MapTexture(handle);
    image = Image::CreateFromData(width, height, texDescriptor->format, static_cast<uint8*>(mappedData));
    rhi::UnmapTexture(handle);

    return image;
}

const TexturesMap& Texture::GetTextureMap()
{
    return textureMap;
}

uint32 Texture::GetDataSize() const
{
    DVASSERT((0 <= texDescriptor->format) && (texDescriptor->format < FORMAT_COUNT));

    uint32 allocSize = width * height * PixelFormatDescriptor::GetPixelFormatSizeInBits(texDescriptor->format) / 8;
    return allocSize;
}

Texture* Texture::CreatePink(rhi::TextureType requestedType, bool checkers)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    // we need instances for pink textures for ResourceEditor. We use it for reloading for different GPUs
    // pink textures at game is invalid situation
    Texture* tex = new Texture();
    if (rhi::TEXTURE_TYPE_CUBE == requestedType)
    {
        tex->texDescriptor->Initialize(rhi::TEXADDR_CLAMP, true);
        tex->texDescriptor->dataSettings.cubefaceFlags = 0x000000FF;
    }
    else
    {
        tex->texDescriptor->Initialize(rhi::TEXADDR_CLAMP, false);
    }

    tex->MakePink(checkers);

    return tex;
}

void Texture::MakePink(bool checkers)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    Vector<Image*>* images = new Vector<Image*>();
    if (texDescriptor->IsCubeMap())
    {
        for (uint32 i = 0; i < Texture::CUBE_FACE_COUNT; ++i)
        {
            Image* img = Image::CreatePinkPlaceholder(checkers);
            img->cubeFaceID = i;
            img->mipmapLevel = 0;

            images->push_back(img);
        }
    }
    else
    {
        images->push_back(Image::CreatePinkPlaceholder(checkers));
    }

    SetParamsFromImages(images);
    FlushDataToRenderer(images);

    isPink = true;

    SetMinMagFilter(rhi::TEXFILTER_NEAREST, rhi::TEXFILTER_NEAREST, rhi::TEXMIPFILTER_NONE);
}

bool Texture::IsPinkPlaceholder()
{
    return isPink;
}

void Texture::SetGPULoadingOrder(const Vector<eGPUFamily>& gpuLoadingOrder_)
{
    gpuLoadingOrder = gpuLoadingOrder_;
}

const Vector<eGPUFamily>& Texture::GetGPULoadingOrder()
{
    return gpuLoadingOrder;
}

eGPUFamily Texture::GetPrimaryGPUForLoading()
{
    if (gpuLoadingOrder.empty())
    {
        return eGPUFamily::GPU_INVALID;
    }

    return gpuLoadingOrder[0];
}

eGPUFamily Texture::GetGPUForLoading(const eGPUFamily requestedGPU, const TextureDescriptor* descriptor)
{
    if (descriptor->IsCompressedFile())
        return descriptor->gpu;

    return requestedGPU;
}

const FilePath& Texture::GetPathname() const
{
    return texDescriptor->pathname;
}

void Texture::SetPathname(const FilePath& path)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    textureMapMutex.Lock();
    textureMap.erase(FILEPATH_MAP_KEY(texDescriptor->pathname));
    texDescriptor->OverridePathName(path);
    if (!texDescriptor->pathname.IsEmpty())
    {
        DVASSERT(textureMap.find(FILEPATH_MAP_KEY(texDescriptor->pathname)) == textureMap.end());
        textureMap[FILEPATH_MAP_KEY(texDescriptor->pathname)] = this;
    }
    textureMapMutex.Unlock();
}

PixelFormat Texture::GetFormat() const
{
    return texDescriptor->format;
}

void Texture::SetPixelization(bool value)
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    //     if (value == pixelizationFlag)
    //     {
    //         return;
    //     }

    pixelizationFlag = value;
    const TexturesMap& texturesMap = GetTextureMap();

    textureMapMutex.Lock();
    for (Map<FilePath, Texture*>::const_iterator iter = texturesMap.begin(); iter != texturesMap.end(); iter++)
    {
        Texture* texture = iter->second;
        rhi::TextureFilter minFilter = pixelizationFlag ? rhi::TextureFilter::TEXFILTER_NEAREST : rhi::TextureFilter(texture->GetDescriptor()->drawSettings.minFilter);
        rhi::TextureFilter magFilter = pixelizationFlag ? rhi::TextureFilter::TEXFILTER_NEAREST : rhi::TextureFilter(texture->GetDescriptor()->drawSettings.magFilter);
        rhi::TextureMipFilter mipFilter = pixelizationFlag ? rhi::TextureMipFilter::TEXMIPFILTER_NONE : rhi::TextureMipFilter(texture->GetDescriptor()->drawSettings.mipFilter);

        texture->SetMinMagFilter(minFilter, magFilter, mipFilter);
    }
    textureMapMutex.Unlock();
    //RHI_COMPLETE
}

uint32 Texture::GetBaseMipMap() const
{
    DAVA_MEMORY_PROFILER_CLASS_ALLOC_SCOPE();

    if (texDescriptor->GetQualityGroup().IsValid())
    {
        const TextureQuality* curTxQuality = QualitySettingsSystem::Instance()->GetTxQuality(QualitySettingsSystem::Instance()->GetCurTextureQuality());
        if (nullptr != curTxQuality)
        {
            return static_cast<uint32>(curTxQuality->albedoBaseMipMapLevel);
        }
    }

    return 0;
}
rhi::HSamplerState Texture::CreateSamplerStateHandle(const rhi::SamplerState::Descriptor::Sampler& samplerState)
{
    rhi::SamplerState::Descriptor samplerDesc;

    samplerDesc.fragmentSampler[0] = samplerState;
    samplerDesc.fragmentSamplerCount = 1;

    return rhi::AcquireSamplerState(samplerDesc);
}
};
