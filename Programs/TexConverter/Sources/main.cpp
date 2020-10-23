#include <TextureCompression/TextureConverter.h>
#include <DocDirSetup/DocDirSetup.h>

#include <CommandLine/ProgramOptions.h>
#include <Debug/DVAssertDefaultHandlers.h>
#include <Engine/Engine.h>
#include <FileSystem/FilePath.h>
#include <FileSystem/VariantType.h>
#include <Logger/Logger.h>
#include <Logger/TeamcityOutput.h>
#include <Render/RenderBase.h>
#include <Render/TextureDescriptor.h>
#include <Render/TextureDescriptorUtils.h>
#include <Render/GPUFamilyDescriptor.h>
#include <Render/Image/Image.h>
#include <Render/Image/ImageSystem.h>
#include <Utils/StringFormat.h>

#include <iostream>
#include <memory>

namespace TexConverterDetails
{
enum eReturnCode : DAVA::int32
{
    SUCCESS = 0,
    ERROR = 1,
};

void PrintMessage(const DAVA::String& message, bool outputToLog)
{
    if (outputToLog == true)
    {
        DAVA::Logger::Info(message.c_str());
    }
    else
    {
        std::cout << message << std::endl;
    }
}

void PrintError(const DAVA::String& message, bool outputToLog)
{
    if (outputToLog == true)
    {
        DAVA::Logger::Error(message.c_str());
    }
    else
    {
        std::cerr << "[error] " << message << std::endl;
    }
}

bool IsImageValidForFormat(const DAVA::ImageInfo& info, const DAVA::PixelFormat format)
{
    if (info.width != info.height && (format == DAVA::FORMAT_PVR2 || format == DAVA::FORMAT_PVR4))
    {
        return false;
    }

    return true;
}

bool IsImageSizeValidForTextures(const DAVA::ImageInfo& info)
{
    return ((info.width >= DAVA::Texture::MINIMAL_WIDTH) && (info.height >= DAVA::Texture::MINIMAL_HEIGHT));
}

bool CanConvertForGPU(DAVA::TextureDescriptor* descriptor, DAVA::eGPUFamily gpu)
{
    using namespace DAVA;

    PixelFormat pixelFormat = descriptor->GetPixelFormatForGPU(gpu);
    if (pixelFormat == PixelFormat::FORMAT_INVALID)
    {
        PrintError("Wrong format for selected GPU", false);
        return false;
    }

    if (GPUFamilyDescriptor::IsGPUForDevice(gpu))
    {
        Vector<FilePath> imagePathnames;
        if (descriptor->IsCubeMap())
        {
            descriptor->GetFacePathnames(imagePathnames);
        }
        else
        {
            imagePathnames.push_back(descriptor->GetSourceTexturePathname());
        }

        for (const FilePath& path : imagePathnames)
        {
            ImageInfo imageInfo = ImageSystem::GetImageInfo(path);
            if (IsImageValidForFormat(imageInfo, pixelFormat) == false)
            {
                String errorMessage = Format("Can't export non-square texture %s into compression format %s",
                                             descriptor->pathname.GetStringValue().c_str(), GlobalEnumMap<PixelFormat>::Instance()->ToString(pixelFormat));
                PrintError(errorMessage, false);
                return false;
            }

            if (IsImageSizeValidForTextures(imageInfo) == false)
            {
                String errorMessage = Format("Can't export small sized texture %s into compression format %s",
                                             descriptor->pathname.GetStringValue().c_str(), GlobalEnumMap<PixelFormat>::Instance()->ToString(pixelFormat));
                PrintError(errorMessage, false);
                return false;
            }
        }
    }
    else
    {
        PrintError("Wrong gpu. Should be gpu for device", false);
        return false;
    }

    return true;
}

DAVA::int32 SplitConvertedTexture(DAVA::TextureDescriptor* descriptor, const DAVA::FilePath& convertedTexturePath, const DAVA::FilePath& outDirPath, DAVA::eGPUFamily gpu)
{
    using namespace DAVA;

    Vector<Image*> loadedImages;
    SCOPE_EXIT
    {
        for (Image* image : loadedImages)
        {
            SafeRelease(image);
        }
    };

    // load and validate
    eErrorCode loadError = ImageSystem::Load(convertedTexturePath, loadedImages);
    if (loadError != eErrorCode::SUCCESS || loadedImages.empty())
    {
        PrintError(Format("Can't load %s", convertedTexturePath.GetStringValue().c_str()), false);
        return eReturnCode::ERROR;
    }

    PixelFormat targetFormat = descriptor->GetPixelFormatForGPU(gpu);
    DVASSERT(targetFormat == loadedImages[0]->format);

    size_t mipmapsCount = loadedImages.size();
    bool isCubemap = loadedImages[0]->cubeFaceID != Texture::INVALID_CUBEMAP_FACE;
    if (isCubemap)
    {
        uint32 firstFace = loadedImages[0]->cubeFaceID;
        mipmapsCount = count_if(loadedImages.begin(), loadedImages.end(), [&firstFace](const Image* img) { return img->cubeFaceID == firstFace; });
    }

    // save textures
    Vector<FilePath> savePathnames;
    descriptor->CreateLoadPathnamesForGPU(gpu, savePathnames);
    size_t outTexturesCount = savePathnames.size();
    if (mipmapsCount < outTexturesCount)
    {
        PrintError(Format("Can't split HD level for %s", convertedTexturePath.GetStringValue().c_str()), false);
        return eReturnCode::ERROR;
    }

    for (FilePath& path : savePathnames)
    { // descriptor - is from source folder, but we should save splited images into out folder
        path.ReplaceDirectory(outDirPath);
    }

    enum class eSavingParam
    {
        SaveOneMip,
        SaveRemainingMips
    };

    auto saveImages = [&](const FilePath& path, size_t mip, eSavingParam param)
    {
        if (isCubemap)
        {
            Vector<Vector<Image*>> savedImages;
            for (uint32 face = 0; face < Texture::CUBE_FACE_COUNT; ++face)
            {
                Vector<Image*> faceMips;

                for (Image* loadedImage : loadedImages)
                {
                    if (loadedImage->cubeFaceID == face && loadedImage->mipmapLevel >= static_cast<DAVA::uint32>(mip))
                    {
                        faceMips.push_back(loadedImage);
                        if (param == eSavingParam::SaveOneMip)
                            break;
                    }
                }

                if (!faceMips.empty())
                {
                    savedImages.resize(savedImages.size() + 1);
                    savedImages.back().swap(faceMips);
                }
            }

            eErrorCode saveError = ImageSystem::SaveAsCubeMap(path, savedImages, targetFormat);
            if (saveError != eErrorCode::SUCCESS)
            {
                PrintError(Format("Can't save %s", path.GetStringValue().c_str()), false);
                return eReturnCode::ERROR;
            }
        }
        else
        {
            DVASSERT(mip < loadedImages.size());
            Vector<Image*> savedImages;
            if (param == eSavingParam::SaveOneMip)
            {
                savedImages.push_back(loadedImages[mip]);
            }
            else
            {
                savedImages.assign(loadedImages.begin() + mip, loadedImages.end());
            }

            eErrorCode saveError = ImageSystem::Save(path, savedImages, targetFormat);
            if (saveError != eErrorCode::SUCCESS)
            {
                PrintError(Format("Can't save %s", path.GetStringValue().c_str()), false);
                return eReturnCode::ERROR;
            }
        }

        return eReturnCode::SUCCESS;
    };

    // save hd mips, each in separate file
    size_t savedMip = 0;
    size_t singleMipCount = outTexturesCount - 1;
    for (size_t mip = 0; mip < singleMipCount; ++mip)
    {
        if (loadedImages[mip]->width > Texture::MINIMAL_WIDTH && loadedImages[mip]->height > Texture::MINIMAL_HEIGHT)
        {
            if (saveImages(savePathnames[mip], savedMip++, eSavingParam::SaveOneMip) == eReturnCode::ERROR)
            {
                return eReturnCode::ERROR;
            }
        }
        else
        {
            break;
        }
    }

    // save remaining mips, all in single file
    size_t lastIndex = savePathnames.size() - 1;
    return saveImages(savePathnames[lastIndex], savedMip, eSavingParam::SaveRemainingMips);
}

DAVA::int32 ConvertTexture(const DAVA::FilePath& texturePath, const DAVA::FilePath& outDirPath, DAVA::eGPUFamily gpu, bool useHD)
{
    using namespace DAVA;
    std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texturePath));
    if (descriptor)
    {
        bool shouldSplitTexture = useHD && descriptor->dataSettings.GetGenerateMipMaps();
        if (CanConvertForGPU(descriptor.get(), gpu) == false)
        {
            return eReturnCode::ERROR;
        }
        FilePath convertedTexturePath = TextureConverter::ConvertTexture(*descriptor.get(), gpu, false, TextureConverter::eConvertQuality::ECQ_VERY_HIGH, outDirPath);
        if (convertedTexturePath == FilePath())
        {
            PrintError("Cannot compress texture", false);
            return eReturnCode::ERROR;
        }

        if (shouldSplitTexture)
        {
            descriptor->dataSettings.SetSeparateHDTextures(true);
            if (SplitConvertedTexture(descriptor.get(), convertedTexturePath, outDirPath, gpu) == eReturnCode::ERROR)
            {
                return eReturnCode::ERROR;
            }
        }

        descriptor->Save(outDirPath + texturePath.GetFilename()); //save converted texture descriptor
        return eReturnCode::SUCCESS;
    }
    else
    {
        PrintError(Format("Cannot create descriptor from %s", texturePath.GetStringValue().c_str()), false);
    }

    return eReturnCode::ERROR;
}

DAVA::int32 ProcessSelftest(bool enableTeamcityLogging)
{
    using namespace DAVA;
    int32 testResult = eReturnCode::SUCCESS;
    FilePath testDirectory = DocumentsDirectorySetup::GetApplicationDocDirectory(GetEngineContext()->fileSystem, "TexConverter");
    FilePath outDirectory = testDirectory + "out/";

    { // set up
        if (enableTeamcityLogging == true)
        {
            GetEngineContext()->logger->SetLogLevel(Logger::LEVEL_INFO);
            GetEngineContext()->logger->AddCustomOutput(new TeamcityOutput());
        }

        //remove data from interrupted tests
        GetEngineContext()->fileSystem->DeleteDirectory(outDirectory, true);
        GetEngineContext()->fileSystem->DeleteDirectory(testDirectory, true);

        //create new data
        DocumentsDirectorySetup::CreateApplicationDocDirectory(GetEngineContext()->fileSystem, "TexConverter");
        DocumentsDirectorySetup::SetApplicationDocDirectory(GetEngineContext()->fileSystem, "TexConverter");
        GetEngineContext()->fileSystem->CreateDirectory(outDirectory);
    }

    { // test
        FilePath texturePath = testDirectory + "image.tex";
        ScopedPtr<Image> image(Image::Create(16, 16, PixelFormat::FORMAT_RGBA8888));

        struct CompressionResult
        {
            String filename;
            uint32 size = 0;
        };
        struct TestData
        {
            eGPUFamily gpu = eGPUFamily::GPU_INVALID;
            ImageFormat imageFormat = ImageFormat::IMAGE_FORMAT_UNKNOWN;
            PixelFormat pixelFormat = PixelFormat::FORMAT_INVALID;
            bool useHD = false;
            bool isNormalMap = false;
            Vector<CompressionResult> result;
        };

        { //single texture
            FilePath pngPath = testDirectory + "image.png";
            {
                ImageSystem::Save(pngPath, image.get());
                TextureDescriptorUtils::CreateDescriptor(pngPath);
            }

            Vector<TestData> environment
            {
              { eGPUFamily::GPU_POWERVR_IOS, ImageFormat::IMAGE_FORMAT_PVR, PixelFormat::FORMAT_RGBA8888, false, true, { { "image.PowerVR_iOS.pvr", 16 } } },
              { eGPUFamily::GPU_POWERVR_IOS, ImageFormat::IMAGE_FORMAT_PVR, PixelFormat::FORMAT_RGBA5551, false, false, { { "image.PowerVR_iOS.pvr", 16 } } },
              { eGPUFamily::GPU_POWERVR_IOS, ImageFormat::IMAGE_FORMAT_PVR, PixelFormat::FORMAT_RGBA4444, false, false, { { "image.PowerVR_iOS.pvr", 16 } } },
              { eGPUFamily::GPU_POWERVR_IOS, ImageFormat::IMAGE_FORMAT_PVR, PixelFormat::FORMAT_RGB888, false, false, { { "image.PowerVR_iOS.pvr", 16 } } },
              { eGPUFamily::GPU_POWERVR_IOS, ImageFormat::IMAGE_FORMAT_PVR, PixelFormat::FORMAT_RGB565, false, false, { { "image.PowerVR_iOS.pvr", 16 } } },
              { eGPUFamily::GPU_POWERVR_IOS, ImageFormat::IMAGE_FORMAT_PVR, PixelFormat::FORMAT_A8, false, false, { { "image.PowerVR_iOS.pvr", 16 } } },
              // we don't compress files into A16 format
              { eGPUFamily::GPU_POWERVR_ANDROID, ImageFormat::IMAGE_FORMAT_PVR, PixelFormat::FORMAT_PVR4, true, true, { { "image.PowerVR_Android.pvr", 8 }, { "image.hd.PowerVR_Android.pvr", 16 } } },
              { eGPUFamily::GPU_POWERVR_ANDROID, ImageFormat::IMAGE_FORMAT_PVR, PixelFormat::FORMAT_PVR2, true, false, { { "image.PowerVR_Android.pvr", 8 }, { "image.hd.PowerVR_Android.pvr", 16 } } },

              { eGPUFamily::GPU_TEGRA, ImageFormat::IMAGE_FORMAT_DDS, PixelFormat::FORMAT_DXT1, false, true, { { "image.tegra.dds", 16 } } },
              { eGPUFamily::GPU_TEGRA, ImageFormat::IMAGE_FORMAT_DDS, PixelFormat::FORMAT_DXT1A, false, false, { { "image.tegra.dds", 16 } } },
              { eGPUFamily::GPU_TEGRA, ImageFormat::IMAGE_FORMAT_DDS, PixelFormat::FORMAT_DXT3, true, false, { { "image.tegra.dds", 8 }, { "image.hd.tegra.dds", 16 } } },

              { eGPUFamily::GPU_MALI, ImageFormat::IMAGE_FORMAT_PVR, PixelFormat::FORMAT_ETC1, true, false, { { "image.mali.pvr", 8 }, { "image.hd.mali.pvr", 16 } } },

              { eGPUFamily::GPU_ADRENO, ImageFormat::IMAGE_FORMAT_DDS, PixelFormat::FORMAT_ATC_RGB, false, false, { { "image.adreno.dds", 16 } } },
              { eGPUFamily::GPU_ADRENO, ImageFormat::IMAGE_FORMAT_DDS, PixelFormat::FORMAT_ATC_RGBA_EXPLICIT_ALPHA, false, true, { { "image.adreno.dds", 16 } } },
              { eGPUFamily::GPU_ADRENO, ImageFormat::IMAGE_FORMAT_DDS, PixelFormat::FORMAT_ATC_RGBA_INTERPOLATED_ALPHA, true, false, { { "image.adreno.dds", 8 }, { "image.hd.adreno.dds", 16 } } },

              { eGPUFamily::GPU_DX11, ImageFormat::IMAGE_FORMAT_DDS, PixelFormat::FORMAT_DXT5, false, false, { { "image.dx11.dds", 16 } } },
              { eGPUFamily::GPU_DX11, ImageFormat::IMAGE_FORMAT_DDS, PixelFormat::FORMAT_DXT5NM, true, true, { { "image.dx11.dds", 8 }, { "image.hd.dx11.dds", 16 } } }
            };

            std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texturePath));
            for (const TestData& data : environment)
            {
                descriptor->Initialize(rhi::TEXADDR_CLAMP, true);
                descriptor->dataSettings.SetIsNormalMap(data.isNormalMap);
                descriptor->compression[data.gpu].imageFormat = data.imageFormat;
                descriptor->compression[data.gpu].format = data.pixelFormat;
                descriptor->Save(texturePath);

                bool allFilesAreCorrect = false;
                if (ConvertTexture(texturePath, outDirectory, data.gpu, data.useHD) == eReturnCode::SUCCESS)
                {
                    allFilesAreCorrect = GetEngineContext()->fileSystem->Exists(outDirectory + texturePath.GetFilename());
                    for (const CompressionResult& result : data.result)
                    {
                        ImageInfo info = ImageSystem::GetImageInfo(outDirectory + result.filename);
                        allFilesAreCorrect = allFilesAreCorrect && (info.format == data.pixelFormat && info.width == result.size);
                    }
                }

                if (allFilesAreCorrect == false)
                {
                    PrintError(Format("Cannot compress texture for gpu: %d, pixelFormat: %d", data.gpu, data.pixelFormat), enableTeamcityLogging);
                    testResult = eReturnCode::ERROR;
                }
                GetEngineContext()->fileSystem->DeleteDirectoryFiles(outDirectory);
            }

            GetEngineContext()->fileSystem->DeleteDirectoryFiles(testDirectory);
        }

        { //cubemap
            Vector<FilePath> imagePathnames
            {
              testDirectory + "image_px.png",
              testDirectory + "image_nx.png",
              testDirectory + "image_py.tga",
              testDirectory + "image_ny.tga",
              testDirectory + "image_pz.png",
              testDirectory + "image_nz.png"
            };

            {
                for (const FilePath& path : imagePathnames)
                {
                    ImageSystem::Save(path, image.get());
                }
                TextureDescriptorUtils::CreateDescriptorCube(texturePath, imagePathnames);
            }

            Vector<TestData> environment
            {
              { eGPUFamily::GPU_TEGRA, ImageFormat::IMAGE_FORMAT_DDS, PixelFormat::FORMAT_DXT5, false, true, { { "image.tegra.dds", 16 } } },
              { eGPUFamily::GPU_POWERVR_IOS, ImageFormat::IMAGE_FORMAT_PVR, PixelFormat::FORMAT_RGBA8888, true, false, { { "image.PowerVR_iOS.pvr", 8 }, { "image.hd.PowerVR_iOS.pvr", 16 } } }
            };

            std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texturePath));
            for (const TestData& data : environment)
            {
                descriptor->Initialize(rhi::TEXADDR_CLAMP, true);
                descriptor->dataSettings.cubefaceFlags = 0x000000FF;

                descriptor->dataSettings.SetIsNormalMap(data.isNormalMap);
                descriptor->compression[data.gpu].imageFormat = data.imageFormat;
                descriptor->compression[data.gpu].format = data.pixelFormat;
                descriptor->Save(texturePath);

                bool allFilesAreCorrect = false;
                if (ConvertTexture(texturePath, outDirectory, data.gpu, data.useHD) == eReturnCode::SUCCESS)
                {
                    allFilesAreCorrect = GetEngineContext()->fileSystem->Exists(outDirectory + texturePath.GetFilename());
                    for (const CompressionResult& result : data.result)
                    {
                        ImageInfo info = ImageSystem::GetImageInfo(outDirectory + result.filename);
                        allFilesAreCorrect = allFilesAreCorrect && (info.format == data.pixelFormat && info.width == result.size && info.faceCount == Texture::CUBE_FACE_COUNT);
                    }
                }

                if (allFilesAreCorrect == false)
                {
                    PrintError(Format("Cannot compress cube texture for gpu: %d, pixelFormat: %d", data.gpu, data.pixelFormat), enableTeamcityLogging);
                    testResult = eReturnCode::ERROR;
                }
                GetEngineContext()->fileSystem->DeleteDirectoryFiles(outDirectory);
            }

            GetEngineContext()->fileSystem->DeleteDirectoryFiles(testDirectory);
        }
    }

    { //clean up
        GetEngineContext()->fileSystem->DeleteDirectory(outDirectory, true);
        GetEngineContext()->fileSystem->DeleteDirectory(testDirectory, true);
        if (enableTeamcityLogging == true)
        {
            GetEngineContext()->logger->SetLogLevel(Logger::LEVEL__DISABLE);
        }
    }

    return testResult;
}

DAVA::int32 ProcessTexConverter(DAVA::Engine& e)
{
    using namespace DAVA;

    const Vector<String>& cmdLine = e.GetCommandLine();

    ProgramOptions helpOption("--help");
    ProgramOptions testOption("--selftest");
    testOption.AddOption("--teamcity", VariantType(false), "Logging in Teamcity format");

    ProgramOptions convertOption("", false);
    convertOption.AddOption("--file", VariantType(String()), "Path to texture (*.tex) file");
    convertOption.AddOption("--gpu", VariantType(String()), "GPU Tag for texture conversion");
    convertOption.AddOption("--outdir", VariantType(String()), "Path to output directory for compressed resources");
    convertOption.AddOption("--hd", VariantType(false), "Use 0-mip level as texture.hd.ext");

    if (helpOption.Parse(cmdLine) == true)
    {
        PrintMessage(convertOption.GetUsageString(), false);
        PrintMessage(testOption.GetUsageString(), false);
        PrintMessage("TexConverter --help --> to see this help", false);
        return eReturnCode::SUCCESS;
    }
    if (testOption.Parse(cmdLine) == true)
    {
        return ProcessSelftest(testOption.GetOption("--teamcity").AsBool());
    }

    if (convertOption.Parse(cmdLine) == true)
    {
        String filePath = convertOption.GetOption("--file").AsString();
        String outDirPath = convertOption.GetOption("--outdir").AsString();
        String gpuName = convertOption.GetOption("--gpu").AsString();
        bool useHD = convertOption.GetOption("--hd").AsBool();

        eGPUFamily gpu = GPUFamilyDescriptor::GetGPUByName(gpuName);
        if (gpu == eGPUFamily::GPU_INVALID)
        {
            PrintError("Wrong gpu name", false);
            return eReturnCode::ERROR;
        }
        else if (gpu == eGPUFamily::GPU_ORIGIN)
        {
            PrintError("Cannot convert to GPU_ORIGIN", false);
            return eReturnCode::ERROR;
        }
        else if (filePath.empty() == true)
        {
            PrintError("Texture path is empty", false);
            return eReturnCode::ERROR;
        }
        else if (outDirPath.empty() == true)
        {
            PrintError("Output folder path is empty", false);
            return eReturnCode::ERROR;
        }

        FilePath dirPath = outDirPath;
        dirPath.MakeDirectoryPathname();
        if (e.GetContext()->fileSystem->Exists(dirPath) == false)
        {
            PrintError("Output folder path does not exist", false);
            return eReturnCode::ERROR;
        }
        FilePath texturePath = filePath;
        if (texturePath.IsEqualToExtension(TextureDescriptor::GetDescriptorExtension()) == false)
        {
            PrintError(Format("Wrong texture pathname (%s). Should be *.tex", filePath.c_str()), false);
            return eReturnCode::ERROR;
        }

        return ConvertTexture(texturePath, dirPath, gpu, useHD);
    }

    PrintError("Wrong command line. Call \"TexConverter --help\" to see help", false);
    return eReturnCode::ERROR;
}
}

int DAVAMain(DAVA::Vector<DAVA::String> cmdLine)
{
    using namespace DAVA;

    Assert::AddHandler(Assert::DefaultLoggerHandler);
    Assert::AddHandler(Assert::DefaultDebuggerBreakHandler);

    Engine e;
    Vector<String> modules;
    e.Init(eEngineRunMode::CONSOLE_MODE, modules, nullptr);

    const EngineContext* context = e.GetContext();
    Logger* logger = context->logger;
    logger->SetLogLevel(Logger::LEVEL__DISABLE);
    logger->EnableConsoleMode();

    e.update.Connect([&e](float32)
                     {
                         DAVA::int32 retCode = TexConverterDetails::ProcessTexConverter(e);
                         e.QuitAsync(retCode);
                     });

    return e.Run();
}
