#include "Render/Image/ImageFormatInterface.h"
#include "FileSystem/File.h"
#include "Utils/Utils.h"

namespace DAVA
{
ImageFormatInterface::ImageFormatInterface(ImageFormat imageFormat_, const String& interfaceName_, const Vector<String>& extensions, const Vector<PixelFormat>& pixelFormats)
    : supportedFormats(pixelFormats)
    , supportedExtensions(extensions)
    , interfaceName(interfaceName_)
    , imageFormat(imageFormat_)
{
}

ImageFormat ImageFormatInterface::GetImageFormat() const
{
    return imageFormat;
}

ImageInfo ImageFormatInterface::GetImageInfo(const FilePath& path) const
{
    ScopedPtr<File> infile(File::Create(path, File::OPEN | File::READ));
    if (infile)
    {
        ImageInfo info = GetImageInfo(infile);
        return info;
    }
    return ImageInfo();
}

bool ImageFormatInterface::IsFormatSupported(PixelFormat format) const
{
    return (std::find(supportedFormats.begin(), supportedFormats.end(), format) != supportedFormats.end());
}

bool ImageFormatInterface::IsFileExtensionSupported(const String& extension) const
{
    for (const String& ext : supportedExtensions)
    {
        if (CompareCaseInsensitive(ext, extension) == 0)
        {
            return true;
        }
    }

    return false;
}

const Vector<String>& ImageFormatInterface::GetExtensions() const
{
    return supportedExtensions;
}

const String& ImageFormatInterface::GetName() const
{
    return interfaceName;
}

bool ImageFormatInterface::CanProcessFile(File* file) const
{
    if (!file)
    {
        DVASSERT(false);
        return false;
    }

    DVASSERT(file->GetPos() == 0);

    bool canProcess = CanProcessFileInternal(file);
    file->Seek(0, File::SEEK_FROM_START);
    return canProcess;
}

bool ImageFormatInterface::CanProcessFileInternal(File* infile) const
{
    return GetImageInfo(infile).dataSize != 0;
}
}
