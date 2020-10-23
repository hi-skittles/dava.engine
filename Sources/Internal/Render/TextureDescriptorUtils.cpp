#include "Render/TextureDescriptorUtils.h"

#include "Engine/Engine.h"
#include "FileSystem/FilePath.h"
#include "FileSystem/FileSystem.h"
#include "Logger/Logger.h"
#include "Render/RenderBase.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"
#include "Render/TextureDescriptor.h"

namespace DAVA
{
bool TextureDescriptorUtils::CreateDescriptor(const DAVA::FilePath& imagePath)
{
    FilePath texturePath = TextureDescriptor::GetDescriptorPathname(imagePath);
    if (GetEngineContext()->fileSystem->Exists(texturePath))
    {
        Logger::Error("%s already exists", texturePath.GetStringValue().c_str());
        return false;
    }

    String imageExtension = imagePath.GetExtension();
    ImageFormat imageFormat = ImageSystem::GetImageFormatForExtension(imageExtension);
    if ((imageFormat == IMAGE_FORMAT_UNKNOWN) || (false == TextureDescriptor::IsSupportedSourceFormat(imageFormat)))
    {
        Logger::Error("Cannot create TextureDescriptor for %s", imagePath.GetStringValue().c_str());
        return false;
    }

    std::unique_ptr<TextureDescriptor> descriptor = std::make_unique<TextureDescriptor>();
    descriptor->pathname = texturePath;

    ImageInfo info = ImageSystem::GetImageInfo(imagePath);
    descriptor->dataSettings.sourceFileFormat = imageFormat;
    descriptor->dataSettings.sourceFileExtension = imageExtension;
    descriptor->compression[eGPUFamily::GPU_ORIGIN].imageFormat = imageFormat;
    descriptor->compression[eGPUFamily::GPU_ORIGIN].format = info.format;
    descriptor->Save(texturePath);
    return true;
}

bool TextureDescriptorUtils::CreateDescriptorCube(const DAVA::FilePath& texturePath, const DAVA::Vector<DAVA::FilePath>& imagePathes)
{
    if (static_cast<uint32>(imagePathes.size()) != Texture::CUBE_FACE_COUNT)
    {
        Logger::Error("Cannot create TextureDescriptor for cube texture from %u source files", static_cast<uint32>(imagePathes.size()));
        return false;
    }

    if (GetEngineContext()->fileSystem->Exists(texturePath))
    {
        Logger::Error("%s already exists", texturePath.GetStringValue().c_str());
        return false;
    }

    std::unique_ptr<TextureDescriptor> descriptor = std::make_unique<TextureDescriptor>();
    descriptor->Initialize(rhi::TEXADDR_CLAMP, true);
    descriptor->dataSettings.cubefaceFlags = 0x000000FF;
    descriptor->pathname = texturePath;

    ImageInfo zeroFaceInfo = ImageSystem::GetImageInfo(imagePathes[0]);
    for (uint32 face = 0; face < Texture::CUBE_FACE_COUNT; ++face)
    {
        String imageExtension = imagePathes[face].GetExtension();
        ImageFormat imageFormat = ImageSystem::GetImageFormatForExtension(imageExtension);
        if ((imageFormat == IMAGE_FORMAT_UNKNOWN) || (false == TextureDescriptor::IsSupportedSourceFormat(imageFormat)))
        {
            Logger::Error("Cannot create cube TextureDescriptor from %s", imagePathes[face].GetStringValue().c_str());
            return false;
        }

        ImageInfo faceInfo = ImageSystem::GetImageInfo(imagePathes[face]);
        if ((faceInfo.width != zeroFaceInfo.width) || (faceInfo.height != zeroFaceInfo.height) || (faceInfo.format != zeroFaceInfo.format))
        {
            Logger::Error("Cannot create cube TextureDescriptor because face %u has different settings", face);
            return false;
        }

        descriptor->dataSettings.cubefaceExtensions[face] = imageExtension;
    }

    Vector<FilePath> faceNames;
    descriptor->GetFacePathnames(faceNames);

    if (faceNames != imagePathes)
    {
        Logger::Error("Cannot create cube TextureDescriptor because imagePathes have unacceptable names");
        return false;
    }

    descriptor->Save(texturePath);
    return true;
}

bool TextureDescriptorUtils::UpdateDescriptor(const DAVA::FilePath& imagePath)
{
    String imageExtension = imagePath.GetExtension();
    ImageFormat imageFormat = ImageSystem::GetImageFormatForExtension(imageExtension);
    if ((imageFormat == IMAGE_FORMAT_UNKNOWN) || (false == TextureDescriptor::IsSupportedSourceFormat(imageFormat)))
    {
        Logger::Error("Cannot update TextureDescriptor for %s", imagePath.GetStringValue().c_str());
        return false;
    }

    FilePath descriptorPath = TextureDescriptor::GetDescriptorPathname(imagePath);
    std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(descriptorPath));
    if (descriptor)
    {
        ImageInfo info = ImageSystem::GetImageInfo(imagePath);
        if (info.format != PixelFormat::FORMAT_INVALID)
        {
            descriptor->dataSettings.sourceFileFormat = imageFormat;
            descriptor->dataSettings.sourceFileExtension = imageExtension;
            descriptor->compression[eGPUFamily::GPU_ORIGIN].imageFormat = imageFormat;
            descriptor->compression[eGPUFamily::GPU_ORIGIN].format = info.format;
            descriptor->Save(descriptorPath);
            return true;
        }
    }
    return false;
}

} // DAVA
