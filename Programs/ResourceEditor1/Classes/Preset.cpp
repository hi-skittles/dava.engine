#include "Preset.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Project/ProjectManagerData.h"

#include "FileSystem/FileSystem.h"
#include "FileSystem/KeyedArchive.h"
#include "Logger/Logger.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"
#include "Render/TextureDescriptor.h"
#include "Render/GPUFamilyDescriptor.h"
#include "Render/Material/NMaterial.h"
#include "Utils/StringFormat.h"

#include "QtTools/FileDialogs/FileDialog.h"

#include "Base/GlobalEnum.h"

namespace Preset
{
using namespace DAVA;

static const String texturePresetFolder = "Preset/Texture/";
static const String materialPresetFolder = "Preset/Material/";
static const QString presetFilter = "Preset (*.preset)";

namespace Internal
{
bool ArePresetDimensionsCorrect(const TextureDescriptor* descriptor, const KeyedArchive* preset)
{
    DVASSERT(preset);
    DVASSERT(descriptor);

    const FilePath sourceImagePath = descriptor->GetSourceTexturePathname();
    const ImageInfo imageInfo = ImageSystem::GetImageInfo(sourceImagePath);
    if (imageInfo.IsEmpty())
    {
        Logger::Error("Can't get image info for %s", sourceImagePath.GetAbsolutePathname().c_str());
        return false;
    }

    DVASSERT(imageInfo.width > 0);
    DVASSERT(imageInfo.height > 0);
    bool dimensionsAreCorrect = true;

    float32 imageRatio = imageInfo.width / imageInfo.height;

    for (uint8 gpu = 0; gpu < GPU_FAMILY_COUNT; ++gpu)
    {
        String gpuName = GPUFamilyDescriptor::GetGPUName(static_cast<eGPUFamily>(gpu));
        const KeyedArchive* compressionArchive = preset->GetArchive(gpuName);
        if (compressionArchive != nullptr)
        {
            if (imageInfo.width != imageInfo.height)
            {
                PixelFormat format = static_cast<PixelFormat>(compressionArchive->GetInt32("format", FORMAT_INVALID));
                if (format == FORMAT_PVR2 || format == FORMAT_PVR4)
                {
                    Logger::Error("Can't apply compression format %s in gpu family %s for non-square texture %s",
                                  GlobalEnumMap<PixelFormat>::Instance()->ToString(format),
                                  GlobalEnumMap<eGPUFamily>::Instance()->ToString(gpu),
                                  sourceImagePath.GetAbsolutePathname().c_str());
                    return false;
                }
            }

            auto compressToWidth = static_cast<uint32>(compressionArchive->GetInt32("width"));
            auto compressToHeight = static_cast<uint32>(compressionArchive->GetInt32("height"));

            if (compressToWidth == 0 && compressToHeight == 0)
                continue;

            DVASSERT(compressToWidth > 0);
            DVASSERT(compressToHeight > 0);

            float32 compressRatio = compressToWidth / compressToHeight;

            if (imageRatio != compressRatio)
            {
                Logger::Error("Preset compression size %u x %u for gpu %s and image size %u x %u have different ratio",
                              compressToWidth, compressToHeight, gpuName.c_str(), imageInfo.width, imageInfo.height);
                dimensionsAreCorrect = false;
            }
            else if ((compressToWidth > imageInfo.width) || (compressToHeight > imageInfo.height))
            {
                Logger::Error("Preset compression size %u x %u for gpu %s is larger than image size %u x %u",
                              compressToWidth, compressToHeight, gpuName.c_str(), imageInfo.width, imageInfo.height);
                dimensionsAreCorrect = false;
            }
            else if (compressToWidth == imageInfo.width && compressToHeight == imageInfo.height)
            {
                Logger::Warning("Preset compression size %u x %u for gpu %s is the same as image size",
                                compressToWidth, compressToHeight, gpuName.c_str());
            }
        }
    }

    return dimensionsAreCorrect;
}

FilePath CreatePresetFolderPathname(const String& folder)
{
    ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
    DVASSERT(data != nullptr);
    const FilePath& projectPath = data->GetProjectPath();
    FilePath folderPath = projectPath + folder;
    folderPath.MakeDirectoryPathname();
    return folderPath;
}

QString GetSavePathname(const QString& caption, const String& folder)
{
    const FilePath folderPath = CreatePresetFolderPathname(folder);
    FileSystem::Instance()->CreateDirectory(folderPath, true);

    return FileDialog::getSaveFileName(nullptr, caption, folderPath.GetAbsolutePathname().c_str(), presetFilter);
}

QString GetOpenPathname(const QString& caption, const String& folder)
{
    const FilePath folderPath = CreatePresetFolderPathname(folder);
    return FileDialog::getOpenFileName(nullptr, caption, folderPath.GetAbsolutePathname().c_str(), presetFilter);
}

} // namespace Internal

bool SaveArchive(const KeyedArchive* presetArchive, const FilePath& path)
{ // We can change the way of saving: as text, as binary, etc..
    DVASSERT(presetArchive != nullptr);
    return presetArchive->SaveToYamlFile(path);
}

KeyedArchive* LoadArchive(const FilePath& path)
{ // We can change the way of loading: as text, as binary, etc..
    KeyedArchive* archive = new KeyedArchive;
    if (path.IsEmpty() || !archive->LoadFromYamlFile(path))
    {
        SafeRelease(archive);
    }

    return archive;
}

bool ApplyTexturePreset(TextureDescriptor* descriptor, const KeyedArchive* preset)
{
    DVASSERT(descriptor != nullptr);
    DVASSERT(preset != nullptr);

    if (descriptor->IsPresetValid(preset) == false)
        return false;

    if (Internal::ArePresetDimensionsCorrect(descriptor, preset) == false)
        return false;

    bool applied = descriptor->DeserializeFromPreset(preset);
    DVASSERT(applied);

    return true;
}

bool DialogSavePresetForTexture(const TextureDescriptor* descriptor)
{
    DVASSERT(descriptor != nullptr);

    ScopedPtr<KeyedArchive> presetArchive(new KeyedArchive());
    if (descriptor->SerializeToPreset(presetArchive) == false)
    {
        Logger::Error("Can't create preset. Check that all GPU convert parameters are valid");
        return false;
    }

    const QString outputFile = Internal::GetSavePathname("Save Texture Preset", texturePresetFolder);
    if (outputFile.isEmpty())
    {
        return false;
    }

    return SaveArchive(presetArchive, outputFile.toStdString());
}

bool DialogLoadPresetForTexture(TextureDescriptor* descriptor)
{
    DVASSERT(descriptor != nullptr);

    const QString inputFile = Internal::GetOpenPathname("Open Texture Preset", texturePresetFolder);
    if (inputFile.isEmpty())
    {
        return false;
    }

    ScopedPtr<KeyedArchive> presetArchive(LoadArchive(inputFile.toStdString()));
    if (!presetArchive)
    {
        Logger::Error("Can't load preset archive");
        return false;
    }

    if (ApplyTexturePreset(descriptor, presetArchive))
    {
        descriptor->Save();
        return true;
    }
    else
    {
        return false;
    }
}

bool DialogLoadPresetForMaterial(NMaterial* material)
{
    return false;
}

bool DialogSavePresetForMaterial(NMaterial* material)
{
    return false;
}
}
