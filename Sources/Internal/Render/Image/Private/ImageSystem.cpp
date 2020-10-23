#include "Render/Image/ImageSystem.h"
#include "Render/Image/ImageFormatInterface.h"
#include "Render/Image/LibJpegHelper.h"
#include "Render/Image/LibDdsHelper.h"
#include "Render/Image/LibPngHelper.h"
#include "Render/Image/LibPVRHelper.h"
#include "Render/Image/LibTgaHelper.h"
#include "Render/Image/LibWebPHelper.h"
#include "Render/Image/LibPSDHelper.h"
#include "Render/Image/LibHDRHelper.h"

#include "Render/Image/ImageConvert.h"
#include "Render/PixelFormatDescriptor.h"
#include "Render/RenderBase.h"

#include "Base/ScopedPtr.h"

#include "FileSystem/FileSystem.h"

#include "Time/SystemTimer.h"
#include "Utils/Utils.h"

namespace DAVA
{
namespace ImageSystem
{
const Array<std::unique_ptr<ImageFormatInterface>, ImageFormat::IMAGE_FORMAT_COUNT>& GetWrappers()
{
    static Array<std::unique_ptr<ImageFormatInterface>, ImageFormat::IMAGE_FORMAT_COUNT> wrappers =
    {
      std::unique_ptr<ImageFormatInterface>(new LibPngHelper()),
      std::unique_ptr<ImageFormatInterface>(new LibDdsHelper()),
      std::unique_ptr<ImageFormatInterface>(new LibPVRHelper()),
      std::unique_ptr<ImageFormatInterface>(new LibJpegHelper()),
      std::unique_ptr<ImageFormatInterface>(new LibTgaHelper()),
      std::unique_ptr<ImageFormatInterface>(new LibWebPHelper()),
      std::unique_ptr<ImageFormatInterface>(new LibPSDHelper()),
      std::unique_ptr<ImageFormatInterface>(new LibHDRHelper())
    };

    return wrappers;
}

Image* LoadSingleMip(const FilePath& pathname, uint32 mip)
{
    Vector<Image*> images;
    ImageSystem::Load(pathname, images, LoadingParams(0, 0, mip, 0));

    Image* image = nullptr;
    if (images.empty() == false)
    {
        image = SafeRetain(images[0]);
        for (Image* img : images)
        {
            SafeRelease(img);
        }
    }

    return image;
}

ImageFormatInterface* GetImageFormatInterface(File* file)
{
    for (const std::unique_ptr<ImageFormatInterface>& wrapper : GetWrappers())
    {
        if (wrapper->CanProcessFile(file))
        {
            return wrapper.get();
        }
    }
    Logger::Error("Can't determinate image format of %s", file->GetFilename().GetAbsolutePathname().c_str());

    return nullptr;
}

Image* EnsurePowerOf2Image(Image* image)
{
    if (IsPowerOf2(image->GetWidth()) && IsPowerOf2(image->GetHeight()))
    {
        return SafeRetain(image);
    }
    Image* newImage = Image::Create(NextPowerOf2(image->GetWidth()),
                                    NextPowerOf2(image->GetHeight()),
                                    image->GetPixelFormat());
    newImage->InsertImage(image, 0, 0);
    return newImage;
}

void EnsurePowerOf2Images(Vector<Image*>& images)
{
    Vector<Image*>::iterator end = images.end();
    for (Vector<Image*>::iterator iter = images.begin(); iter != end; ++iter)
    {
        Image* image = (*iter);
        if (!IsPowerOf2(image->GetWidth()) || !IsPowerOf2(image->GetHeight()))
        {
            Image* newImage = Image::Create(NextPowerOf2(image->GetWidth()),
                                            NextPowerOf2(image->GetHeight()),
                                            image->GetPixelFormat());
            newImage->InsertImage(image, 0, 0);
            (*iter) = newImage;
            SafeRelease(image);
        }
    }
}

eErrorCode Load(const FilePath& pathname, Vector<Image*>& imageSet, const LoadingParams& loadingParams)
{
    ScopedPtr<File> fileRead(File::Create(pathname, File::READ | File::OPEN));
    if (!fileRead)
    {
        return eErrorCode::ERROR_FILE_NOTFOUND;
    }

    eErrorCode result = Load(fileRead, imageSet, loadingParams);
    return result;
}

eErrorCode Load(File* file, Vector<Image*>& imageSet, const LoadingParams& loadingParams)
{
    ImageFormatInterface* properWrapper = GetImageFormatInterface(file->GetFilename()); //fast by filename
    if (nullptr == properWrapper)
    {
        // Retry by content.
        properWrapper = GetImageFormatInterface(file); //slow by content
    }

    if (nullptr == properWrapper)
    {
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    return properWrapper->ReadFile(file, imageSet, loadingParams);
}

eErrorCode SaveAsCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat compressionFormat, ImageQuality quality)
{
    ImageFormatInterface* properWrapper = GetImageFormatInterface(fileName);
    if (nullptr == properWrapper)
    {
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    return properWrapper->WriteFileAsCubeMap(fileName, imageSet, compressionFormat, quality);
}

eErrorCode Save(const FilePath& fileName, const Vector<Image*>& imageSet, PixelFormat compressionFormat, ImageQuality quality)
{
    ImageFormatInterface* properWrapper = GetImageFormatInterface(fileName);
    if (nullptr == properWrapper)
    {
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    return properWrapper->WriteFile(fileName, imageSet, compressionFormat, quality);
}

eErrorCode Save(const FilePath& fileName, Image* image, PixelFormat compressionFormat, ImageQuality quality)
{
    DVASSERT(image != nullptr);

    Vector<Image*> imageSet;
    imageSet.push_back(image);
    return Save(fileName, imageSet, compressionFormat, quality);
}

ImageFormatInterface* GetImageFormatInterface(const FilePath& pathName)
{
    const String extension = pathName.GetExtension();
    for (const std::unique_ptr<ImageFormatInterface>& wrapper : GetWrappers())
    {
        if (wrapper->IsFileExtensionSupported(extension))
        {
            return wrapper.get();
        }
    }

    return nullptr;
}

ImageFormat GetImageFormatForExtension(const FilePath& pathname)
{
    return GetImageFormatForExtension(pathname.GetExtension());
}

ImageFormat GetImageFormatForExtension(const String& extension)
{
    for (const std::unique_ptr<ImageFormatInterface>& wrapper : GetWrappers())
    {
        if (wrapper->IsFileExtensionSupported(extension))
            return wrapper->GetImageFormat();
    }

    return IMAGE_FORMAT_UNKNOWN;
}

ImageFormat GetImageFormatByName(const String& name)
{
    for (const std::unique_ptr<ImageFormatInterface>& wrapper : GetWrappers())
    {
        if (CompareCaseInsensitive(wrapper->GetName(), name) == 0)
            return wrapper->GetImageFormat();
    }

    return IMAGE_FORMAT_UNKNOWN;
}

ImageInfo GetImageInfo(File* infile)
{
    DVASSERT(infile != nullptr);

    const ImageFormatInterface* properWrapper = GetImageFormatInterface(infile);
    if (nullptr != properWrapper)
    {
        infile->Seek(0, File::SEEK_FROM_START); //reset file state after GetImageFormatInterface
        return properWrapper->GetImageInfo(infile);
    }

    return ImageInfo();
}

ImageInfo GetImageInfo(const FilePath& pathName)
{
    ImageFormatInterface* properWrapper = GetImageFormatInterface(pathName); //fast by pathname
    if (nullptr == properWrapper)
    {
        ScopedPtr<File> infile(File::Create(pathName, File::OPEN | File::READ));
        if (infile)
        {
            return GetImageInfo(infile);
        }

        return ImageInfo();
    }

    return properWrapper->GetImageInfo(pathName);
}

uint32 GetBaseMipmap(const LoadingParams& sourceImageParams, const LoadingParams& loadingParams)
{
    if (sourceImageParams.minimalWidth != 0 || sourceImageParams.minimalHeight != 0)
    {
        uint32 width = sourceImageParams.minimalWidth;
        uint32 height = sourceImageParams.minimalHeight;
        uint32 fromMipMap = sourceImageParams.baseMipmap;

        while ((((width >> fromMipMap) < loadingParams.minimalWidth) || ((height >> fromMipMap) < loadingParams.minimalHeight)) && fromMipMap != 0)
        {
            --fromMipMap;
        }

        return fromMipMap;
    }

    return sourceImageParams.baseMipmap;
}

ImageFormatInterface* GetImageFormatInterface(ImageFormat fileFormat)
{
    DVASSERT(fileFormat < IMAGE_FORMAT_COUNT);
    return GetWrappers()[fileFormat].get();
}

const Vector<String>& GetExtensionsFor(ImageFormat format)
{
    return GetImageFormatInterface(format)->GetExtensions();
}

const String& GetDefaultExtension(ImageFormat format)
{
    return GetExtensionsFor(format)[0];
}
}
}
