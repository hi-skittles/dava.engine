#include "ImageTools/ImageTools.h"

#include "Base/GlobalEnum.h"
#include "Render/GPUFamilyDescriptor.h"
#include "Render/PixelFormatDescriptor.h"
#include "Render/Image/ImageConvert.h"

#include "Main/QtUtils.h"

namespace ImageTools
{
void SaveImage(DAVA::Image* image, const DAVA::FilePath& pathname)
{
    DAVA::ImageSystem::Save(pathname, image, image->format);
}

DAVA::Image* LoadImage(const DAVA::FilePath& pathname)
{
    return DAVA::ImageSystem::LoadSingleMip(pathname);
}

DAVA::uint32 GetTexturePhysicalSize(const DAVA::TextureDescriptor* descriptor, const DAVA::eGPUFamily forGPU, DAVA::uint32 baseMipMaps)
{
    DAVA::uint32 size = 0;

    DAVA::Vector<DAVA::FilePath> files;

    if (descriptor->IsCubeMap() && forGPU == DAVA::GPU_ORIGIN)
    {
        DAVA::Vector<DAVA::FilePath> faceNames;
        descriptor->GetFacePathnames(faceNames);

        files.reserve(faceNames.size());
        for (auto& faceName : faceNames)
        {
            if (!faceName.IsEmpty())
                files.push_back(faceName);
        }
    }
    else
    {
        descriptor->CreateLoadPathnamesForGPU(forGPU, files);
    }

    DAVA::FileSystem* fileSystem = DAVA::FileSystem::Instance();
    for (const DAVA::FilePath& imagePathname : files)
    {
        if (fileSystem->Exists(imagePathname) && fileSystem->IsFile(imagePathname))
        {
            DAVA::ImageInfo info = DAVA::ImageSystem::GetImageInfo(imagePathname);
            if (!info.IsEmpty())
            {
                DAVA::uint32 m = DAVA::Min(baseMipMaps, info.mipmapsCount - 1);
                for (; m < info.mipmapsCount; ++m)
                {
                    DAVA::uint32 w = DAVA::Max(info.width >> m, 1u);
                    DAVA::uint32 h = DAVA::Max(info.height >> m, 1u);
                    size += DAVA::ImageUtils::GetSizeInBytes(w, h, info.format);
                }
            }
            else
            {
                DAVA::Logger::Error("ImageTools::[GetTexturePhysicalSize] Can't detect type of file %s", imagePathname.GetStringValue().c_str());
            }
        }
    }

    return size;
}

void ConvertImage(const DAVA::TextureDescriptor* descriptor, const DAVA::eGPUFamily forGPU, DAVA::TextureConverter::eConvertQuality quality)
{
    if (!descriptor || descriptor->compression[forGPU].format == DAVA::FORMAT_INVALID)
    {
        return;
    }

    DAVA::TextureConverter::ConvertTexture(*descriptor, forGPU, true, quality);
}

bool SplitImage(const DAVA::FilePath& pathname)
{
    DAVA::ScopedPtr<DAVA::Image> loadedImage(DAVA::ImageSystem::LoadSingleMip(pathname));
    if (!loadedImage)
    {
        DAVA::Logger::Error("Can't load image %s", pathname.GetAbsolutePathname().c_str());
        return false;
    }

    if (loadedImage->GetPixelFormat() != DAVA::FORMAT_RGBA8888)
    {
        DAVA::Logger::Error("Incorrect image format %s. Must be RGBA8888", DAVA::PixelFormatDescriptor::GetPixelFormatString(loadedImage->GetPixelFormat()));
        return false;
    }

    Channels channels = CreateSplittedImages(loadedImage);

    DAVA::FilePath folder(pathname.GetDirectory());

    SaveImage(channels.red, folder + "r.png");
    SaveImage(channels.green, folder + "g.png");
    SaveImage(channels.blue, folder + "b.png");
    SaveImage(channels.alpha, folder + "a.png");

    return true;
}

bool MergeImages(const DAVA::FilePath& folder)
{
    DVASSERT(folder.IsDirectoryPathname());

    DAVA::ScopedPtr<DAVA::Image> r(LoadImage(folder + "r.png"));
    DAVA::ScopedPtr<DAVA::Image> g(LoadImage(folder + "g.png"));
    DAVA::ScopedPtr<DAVA::Image> b(LoadImage(folder + "b.png"));
    DAVA::ScopedPtr<DAVA::Image> a(LoadImage(folder + "a.png"));
    Channels channels(r, g, b, a);

    if (channels.IsEmpty())
    {
        DAVA::Logger::Error("Can't load one or more channel images from folder %s", folder.GetAbsolutePathname().c_str());
        return false;
    }

    if (!channels.HasFormat(DAVA::FORMAT_A8))
    {
        DAVA::Logger::Error("Can't merge images. Source format must be Grayscale 8bit");
        return false;
    }

    if (!channels.ChannelesResolutionEqual())
    {
        DAVA::Logger::Error("Can't merge images. Source images must have same size");
        return false;
    }

    DAVA::ScopedPtr<DAVA::Image> mergedImage(CreateMergedImage(channels));

    DAVA::ImageSystem::Save(folder + "merged.png", mergedImage);
    return true;
}

Channels CreateSplittedImages(DAVA::Image* originalImage)
{
    DAVA::ScopedPtr<DAVA::Image> r(DAVA::Image::Create(originalImage->width, originalImage->height, DAVA::FORMAT_A8));
    DAVA::ScopedPtr<DAVA::Image> g(DAVA::Image::Create(originalImage->width, originalImage->height, DAVA::FORMAT_A8));
    DAVA::ScopedPtr<DAVA::Image> b(DAVA::Image::Create(originalImage->width, originalImage->height, DAVA::FORMAT_A8));
    DAVA::ScopedPtr<DAVA::Image> a(DAVA::Image::Create(originalImage->width, originalImage->height, DAVA::FORMAT_A8));

    DAVA::int32 size = originalImage->width * originalImage->height;
    DAVA::int32 pixelSizeInBytes = DAVA::PixelFormatDescriptor::GetPixelFormatSizeInBits(DAVA::FORMAT_RGBA8888) / 8;
    for (DAVA::int32 i = 0; i < size; ++i)
    {
        DAVA::int32 offset = i * pixelSizeInBytes;
        r->data[i] = originalImage->data[offset];
        g->data[i] = originalImage->data[offset + 1];
        b->data[i] = originalImage->data[offset + 2];
        a->data[i] = originalImage->data[offset + 3];
    }
    return Channels(r, g, b, a);
}

DAVA::Image* CreateMergedImage(const Channels& channels)
{
    if (!channels.ChannelesResolutionEqual() || !channels.HasFormat(DAVA::FORMAT_A8))
    {
        return nullptr;
    }
    DAVA::Image* mergedImage = DAVA::Image::Create(channels.red->width, channels.red->height, DAVA::FORMAT_RGBA8888);
    DAVA::int32 size = mergedImage->width * mergedImage->height;
    DAVA::int32 pixelSizeInBytes = DAVA::PixelFormatDescriptor::GetPixelFormatSizeInBits(DAVA::FORMAT_RGBA8888) / 8;
    for (DAVA::int32 i = 0; i < size; ++i)
    {
        DAVA::int32 offset = i * pixelSizeInBytes;
        mergedImage->data[offset] = channels.red->data[i];
        mergedImage->data[offset + 1] = channels.green->data[i];
        mergedImage->data[offset + 2] = channels.blue->data[i];
        mergedImage->data[offset + 3] = channels.alpha->data[i];
    }
    return mergedImage;
}

void SetChannel(DAVA::Image* image, eComponentsRGBA channel, DAVA::uint8 value)
{
    if (image->format != DAVA::FORMAT_RGBA8888)
    {
        return;
    }
    DAVA::int32 size = image->width * image->height;
    static const DAVA::int32 pixelSizeInBytes = 4;
    DAVA::int32 offset = channel;
    for (DAVA::int32 i = 0; i < size; ++i, offset += pixelSizeInBytes)
    {
        image->data[offset] = value;
    }
}

QImage FromDavaImage(const DAVA::FilePath& pathname)
{
    auto image = LoadImage(pathname);
    if (image)
    {
        QImage img = FromDavaImage(image);
        SafeRelease(image);

        return img;
    }

    return QImage();
}

QImage FromDavaImage(const DAVA::Image* image)
{
    DVASSERT(image != nullptr);

    if (image->format == DAVA::FORMAT_RGBA8888)
    {
        QImage qtImage(image->width, image->height, QImage::Format_RGBA8888);
        Memcpy(qtImage.bits(), image->data, image->dataSize);
        return qtImage;
    }
    else if (DAVA::ImageConvert::CanConvertFromTo(image->format, DAVA::FORMAT_RGBA8888))
    {
        DAVA::ScopedPtr<DAVA::Image> newImage(DAVA::Image::Create(image->width, image->height, DAVA::FORMAT_RGBA8888));
        bool converted = DAVA::ImageConvert::ConvertImage(image, newImage);
        if (converted)
        {
            return FromDavaImage(newImage);
        }
        else
        {
            DAVA::Logger::Error("[%s]: Error converting from %s", __FUNCTION__, GlobalEnumMap<DAVA::PixelFormat>::Instance()->ToString(image->format));
            return QImage();
        }
    }
    else
    {
        DAVA::Logger::Error("[%s]: Converting from %s is not implemented", __FUNCTION__, GlobalEnumMap<DAVA::PixelFormat>::Instance()->ToString(image->format));
        return QImage();
    }
}

} // namespace ImageTools
