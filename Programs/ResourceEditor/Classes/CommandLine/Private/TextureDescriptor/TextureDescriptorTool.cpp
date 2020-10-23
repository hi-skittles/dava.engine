#include "Classes/CommandLine/TextureDescriptorTool.h"

#include <REPlatform/CommandLine/OptionName.h>
#include <REPlatform/Scene/Utils/RETextureDescriptorUtils.h>

#include <TArc/Utils/ModuleCollection.h>

#include <FileSystem/File.h>
#include <Render/GPUFamilyDescriptor.h>
#include <Render/PixelFormatDescriptor.h>
#include <Utils/StringFormat.h>

namespace TextureDescriptorToolDetail
{
DAVA::Vector<DAVA::FilePath> LoadPathesFromFile(const DAVA::FilePath& filePath)
{
    using namespace DAVA;

    ScopedPtr<File> fileWithPathes(File::Create(filePath, File::OPEN | File::READ));
    if (!fileWithPathes)
    {
        Logger::Error("Can't open file %s", filePath.GetStringValue().c_str());
        return Vector<FilePath>();
    }

    Vector<FilePath> pathes;
    do
    {
        String path = fileWithPathes->ReadLine();
        if (path.empty())
        {
            Logger::Warning("Found empty string in file %s", filePath.GetStringValue().c_str());
            break;
        }
        pathes.emplace_back(path);
    } while (!fileWithPathes->IsEof());

    return pathes;
}
}

TextureDescriptorTool::TextureDescriptorTool(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "-texdescriptor")
{
    using namespace DAVA;

    options.AddOption(OptionName::Folder, VariantType(String("")), "Path to folder for operation on descriptors");
    options.AddOption(OptionName::File, VariantType(String("")), "Pathname of descriptor");

    options.AddOption(OptionName::ProcessFileList, VariantType(String("")), "Pathname to file with descriptor pathes");
    options.AddOption(OptionName::PresetsList, VariantType(String("")), "Pathname to file with yaml pathes");

    options.AddOption(OptionName::Resave, VariantType(false), "Resave descriptor files in target folder");
    options.AddOption(OptionName::Create, VariantType(false), "Create descriptors for image files");
    options.AddOption(OptionName::SetCompression, VariantType(false), "Set compression parameters for descriptor or for all descriptors in folder");
    options.AddOption(OptionName::SetPreset, VariantType(false), "Update descriptor(s) with given preset data");
    options.AddOption(OptionName::SavePreset, VariantType(false), "Save preset of descriptor(s)");

    options.AddOption(OptionName::Force, VariantType(false), "Enables force running of selected operation");
    options.AddOption(OptionName::Mipmaps, VariantType(false), "Enables generation of mipmaps");
    options.AddOption(OptionName::Convert, VariantType(false), "Runs compression of texture after setting of compression parameters");
    options.AddOption(OptionName::Quality, VariantType(static_cast<uint32>(TextureConverter::ECQ_DEFAULT)), "Quality of pvr/etc compression. Default is 4 - the best quality. Available values [0-4]");
    options.AddOption(OptionName::PresetOpt, VariantType(String("")), "Uses preset for an operation");

    //GPU
    for (uint8 gpu = GPU_POWERVR_IOS; gpu < GPU_DEVICE_COUNT; ++gpu)
    {
        eGPUFamily gpuFamily = static_cast<eGPUFamily>(gpu);
        options.AddOption(OptionName::MakeNameForGPU(gpuFamily), VariantType(String("")), Format("Pixel format for %s gpu", GPUFamilyDescriptor::GetGPUName(gpuFamily).c_str()), true);
    }
}

bool TextureDescriptorTool::PostInitInternal()
{
    ReadCommandLine();
    return ValidateCommandLine();
}

void TextureDescriptorTool::ReadCommandLine()
{
    using namespace DAVA;

    folderPathname = options.GetOption(OptionName::Folder).AsString();
    filePathname = options.GetOption(OptionName::File).AsString();
    filesList = options.GetOption(OptionName::ProcessFileList).AsString();
    presetPath = options.GetOption(OptionName::PresetOpt).AsString();
    presetsList = options.GetOption(OptionName::PresetsList).AsString();

    const uint32 qualityValue = options.GetOption(OptionName::Quality).AsUInt32();
    quality = Clamp(static_cast<TextureConverter::eConvertQuality>(qualityValue), TextureConverter::ECQ_FASTEST, TextureConverter::ECQ_VERY_HIGH);

    if (options.GetOption(OptionName::Resave).AsBool())
    {
        commandAction = ACTION_RESAVE_DESCRIPTORS;
    }
    else if (options.GetOption(OptionName::Create).AsBool())
    {
        commandAction = ACTION_CREATE_DESCRIPTORS;
    }
    else if (options.GetOption(OptionName::SetCompression).AsBool())
    {
        commandAction = ACTION_SET_COMPRESSION;
    }
    else if (options.GetOption(OptionName::SetPreset).AsBool())
    {
        commandAction = ACTION_SET_PRESET;
    }
    else if (options.GetOption(OptionName::SavePreset).AsBool())
    {
        commandAction = ACTION_SAVE_PRESET;
    }

    forceModeEnabled = options.GetOption(OptionName::Force).AsBool();
    convertEnabled = options.GetOption(OptionName::Convert).AsBool();
    generateMipMaps = options.GetOption(OptionName::Mipmaps).AsBool();

    for (uint8 gpu = GPU_POWERVR_IOS; gpu < GPU_DEVICE_COUNT; ++gpu)
    {
        const eGPUFamily gpuFamily = static_cast<eGPUFamily>(gpu);
        const String optionName = OptionName::MakeNameForGPU(gpuFamily);

        DAVA::VariantType gpuOption = options.GetOption(optionName);

        const FastName formatName(options.GetOption(optionName).AsString().c_str());
        const PixelFormat pixelFormat = PixelFormatDescriptor::GetPixelFormatByName(formatName);

        if (pixelFormat != FORMAT_INVALID)
        {
            TextureDescriptor::Compression compression;
            compression.format = pixelFormat;
            compression.imageFormat = GPUFamilyDescriptor::GetCompressedFileFormat(gpuFamily, pixelFormat);
            compression.compressToWidth = compression.compressToHeight = 0;
            if (options.GetOptionValuesCount(optionName) > 2)
            {
                const String widthStr = options.GetOption(optionName, 1).AsString();
                const String heightStr = options.GetOption(optionName, 2).AsString();
                if (!widthStr.empty() && !heightStr.empty())
                {
                    compression.compressToWidth = atoi(widthStr.c_str());
                    compression.compressToHeight = atoi(heightStr.c_str());

                    if (compression.compressToWidth < 0 || compression.compressToHeight < 0)
                    {
                        Logger::Error("Wrong size parameters for gpu: %s", optionName.c_str());
                        compression.compressToWidth = compression.compressToHeight = 0;
                    }
                }
            }

            compressionParams[gpuFamily] = compression;
        }
    }
}

bool TextureDescriptorTool::ValidateCommandLine()
{
    if (commandAction == TextureDescriptorTool::ACTION_NONE)
    {
        DAVA::Logger::Error("Action was not specified");
        return false;
    }

    if (commandAction == TextureDescriptorTool::ACTION_SAVE_PRESET)
    {
        if ((!filePathname.IsEmpty() && presetPath.IsEmpty()) || (filePathname.IsEmpty() && !presetPath.IsEmpty()))
        {
            DAVA::Logger::Error("File or preset parameter was not specified");
            return false;
        }
        else if ((!filesList.IsEmpty() && presetsList.IsEmpty()) || (filesList.IsEmpty() && !presetsList.IsEmpty()))
        {
            DAVA::Logger::Error("FilesList or presetsList parameter was not specified");
            return false;
        }
    }
    else
    {
        if (filePathname.IsEmpty() && folderPathname.IsEmpty())
        {
            DAVA::Logger::Error("File or folder parameter was not specified");
            return false;
        }

        if (!folderPathname.IsEmpty())
        {
            folderPathname.MakeDirectoryPathname();
        }

        if ((commandAction == TextureDescriptorTool::ACTION_SET_COMPRESSION) && compressionParams.empty())
        {
            DAVA::Logger::Error("GPU params were not specified");
            return false;
        }
        else if ((commandAction == TextureDescriptorTool::ACTION_SET_PRESET) && presetPath.IsEmpty())
        {
            DAVA::Logger::Error("Preset was not specified");
            return false;
        }
    }

    return true;
}

DAVA::ConsoleModule::eFrameResult TextureDescriptorTool::OnFrameInternal()
{
    switch (commandAction)
    {
    case ACTION_RESAVE_DESCRIPTORS:
    {
        if (folderPathname.IsEmpty())
        {
            DAVA::RETextureDescriptorUtils::ResaveDescriptor(filePathname);
        }
        else
        {
            DAVA::RETextureDescriptorUtils::ResaveDescriptorsForFolder(folderPathname);
        }

        break;
    }
    case ACTION_CREATE_DESCRIPTORS:
    {
        if (folderPathname.IsEmpty())
        {
            DAVA::RETextureDescriptorUtils::CreateOrUpdateDescriptor(filePathname, presetPath);
        }
        else
        {
            DAVA::RETextureDescriptorUtils::CreateDescriptorsForFolder(folderPathname, presetPath);
        }
        break;
    }
    case ACTION_SET_COMPRESSION:
    {
        if (folderPathname.IsEmpty())
        {
            DAVA::RETextureDescriptorUtils::SetCompressionParams(filePathname, compressionParams, convertEnabled, forceModeEnabled, quality, generateMipMaps);
        }
        else
        {
            DAVA::RETextureDescriptorUtils::SetCompressionParamsForFolder(folderPathname, compressionParams, convertEnabled, forceModeEnabled, quality, generateMipMaps);
        }
        break;
    }
    case ACTION_SET_PRESET:
    {
        if (folderPathname.IsEmpty())
        {
            DAVA::RETextureDescriptorUtils::SetPreset(filePathname, presetPath, convertEnabled, quality);
        }
        else
        {
            DAVA::RETextureDescriptorUtils::SetPresetForFolder(folderPathname, presetPath, convertEnabled, quality);
        }
        break;
    }
    case ACTION_SAVE_PRESET:
    {
        if (!filesList.IsEmpty())
        {
            DAVA::RETextureDescriptorUtils::SavePreset(TextureDescriptorToolDetail::LoadPathesFromFile(filesList), TextureDescriptorToolDetail::LoadPathesFromFile(presetsList));
        }
        else
        {
            DAVA::RETextureDescriptorUtils::SavePreset({ filePathname }, { presetPath });
        }
        break;
    }
    default:
    {
        DAVA::Logger::Error("Unhandled action!");
        break;
    }
    }

    return DAVA::ConsoleModule::eFrameResult::FINISHED;
}

void TextureDescriptorTool::ShowHelpInternal()
{
    CommandLineModule::ShowHelpInternal();

    DAVA::Logger::Info("Examples:");
    DAVA::Logger::Info("\t-texdescriptor -create -folder /Users/SmokeTest/DataSource/3d/Maps/images/");
    DAVA::Logger::Info("\t-texdescriptor -create -folder /Users/SmokeTest/DataSource/3d/Maps/images/ -preset /Users/descriptor.yaml");
    DAVA::Logger::Info("\t-texdescriptor -create -file /Users/SmokeTest/DataSource/3d/Maps/images/texture.png");

    DAVA::Logger::Info("\t-texdescriptor -resave -folder /Users/SmokeTest/DataSource/3d/Maps/images/");
    DAVA::Logger::Info("\t-texdescriptor -resave -file /Users/SmokeTest/DataSource/3d/Maps/images/texture.tex");

    DAVA::Logger::Info("\t-texdescriptor -setcompression -folder /Users/SmokeTest/DataSource/3d/Maps/images/");
    DAVA::Logger::Info("\t-texdescriptor -setcompression -file /Users/SmokeTest/DataSource/3d/Maps/images/texture.tex -adreno ATC_RGB -tegra DXT5 -mali ETC1 -convert -m -quality 4");

    DAVA::Logger::Info("\t-texdescriptor -setpreset -folder /Users/SmokeTest/DataSource/3d/Maps/images/ -preset /Users/descriptor.yaml");
    DAVA::Logger::Info("\t-texdescriptor -setpreset -file /Users/SmokeTest/DataSource/3d/Maps/images/texture.tex -preset /Users/descriptor.yaml -convert -qulity 4");

    DAVA::Logger::Info("\t-texdescriptor -savepreset -processfilelist /Users/textures.txt -presetslist /Users/presets.txt");
    DAVA::Logger::Info("\t-texdescriptor -savepreset -file /Users/SmokeTest/DataSource/3d/Maps/images/texture.tex");
}

DECL_TARC_MODULE(TextureDescriptorTool);
