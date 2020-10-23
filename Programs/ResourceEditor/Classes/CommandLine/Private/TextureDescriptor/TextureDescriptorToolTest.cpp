#include "Classes/CommandLine/TextureDescriptorTool.h"

#include <REPlatform/CommandLine/CommandLineModuleTestUtils.h>
#include <REPlatform/Scene/Utils/RETextureDescriptorUtils.h>

#include <TArc/Testing/ConsoleModuleTestExecution.h>
#include <TArc/Testing/TArcUnitTests.h>

#include <Base/BaseTypes.h>
#include <FileSystem/FilePath.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/KeyedArchive.h>
#include <Render/Image/Image.h>
#include <Render/Image/ImageSystem.h>
#include <Render/RenderBase.h>
#include <Render/TextureDescriptor.h>
#include <Utils/Random.h>

#include <memory>

namespace TDTestDetail
{
const DAVA::String testFolderStr = "~doc:/Test/TextureDescriptorTool/";

bool CreateImageFile(const DAVA::FilePath& imagePathname)
{
    using namespace DAVA;

    ScopedPtr<Image> image(Image::Create(64u, 64u, PixelFormat::FORMAT_RGBA8888));
    FileSystem::Instance()->CreateDirectory(imagePathname.GetDirectory(), true);
    eErrorCode saved = ImageSystem::Save(imagePathname, image, image->format);
    return (saved == eErrorCode::SUCCESS);
}

bool SavePreset(DAVA::TextureDescriptor* descriptor, const DAVA::FilePath& pathname)
{
    DAVA::ScopedPtr<DAVA::KeyedArchive> presetArchive(new DAVA::KeyedArchive());
    if (descriptor->SerializeToPreset(presetArchive) == false)
    {
        return false;
    }
    return presetArchive->SaveToYamlFile(pathname);
}
}

DAVA_TARC_TESTCLASS(TextureDescriptorToolTest)
{
    DAVA_TEST (CreateTest)
    {
        using namespace DAVA;

        CommandLineModuleTestUtils::ClearTestFolder(TDTestDetail::testFolderStr);

        FilePath imagePathname = TDTestDetail::testFolderStr + "123/image.tga";
        FilePath texPathname = TDTestDetail::testFolderStr + "123/image.tex";
        TEST_VERIFY(TDTestDetail::CreateImageFile(imagePathname));

        std::unique_ptr<TextureDescriptor> sourceDescriptor(new TextureDescriptor());
        sourceDescriptor->compression[eGPUFamily::GPU_MALI].format = PixelFormat::FORMAT_RGB888;

        FilePath presetPathname = TDTestDetail::testFolderStr + "preset.yaml";
        TEST_VERIFY(TDTestDetail::SavePreset(sourceDescriptor.get(), presetPathname));

        { // file with preset

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-texdescriptor",
              "-create",
              "-file",
              imagePathname.GetAbsolutePathname(),
              "-preset",
              presetPathname.GetAbsolutePathname()
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<TextureDescriptorTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texPathname));
            if (descriptor)
            {
                TEST_VERIFY(descriptor->dataSettings.sourceFileFormat == ImageFormat::IMAGE_FORMAT_TGA);
                TEST_VERIFY(descriptor->dataSettings.sourceFileExtension == ".tga");
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_MALI].format == PixelFormat::FORMAT_RGB888);

                FileSystem::Instance()->DeleteFile(texPathname);
            }
            else
            {
                TEST_VERIFY(false);
            }
        }

        { // folder
            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-texdescriptor",
              "-create",
              "-folder",
              FilePath(TDTestDetail::testFolderStr).GetAbsolutePathname(),
              "-preset",
              presetPathname.GetAbsolutePathname()
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<TextureDescriptorTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texPathname));
            if (descriptor)
            {
                TEST_VERIFY(descriptor->dataSettings.sourceFileFormat == ImageFormat::IMAGE_FORMAT_TGA);
                TEST_VERIFY(descriptor->dataSettings.sourceFileExtension == ".tga");
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_MALI].format == PixelFormat::FORMAT_RGB888);

                FileSystem::Instance()->DeleteFile(texPathname);
            }
            else
            {
                TEST_VERIFY(false);
            }
        }

        CommandLineModuleTestUtils::ClearTestFolder(TDTestDetail::testFolderStr);
    }

    DAVA_TEST (ResaveTest)
    {
        using namespace DAVA;

        CommandLineModuleTestUtils::ClearTestFolder(TDTestDetail::testFolderStr);

        FilePath texPathname = TDTestDetail::testFolderStr + "123/image.tex";
        FileSystem::Instance()->CreateDirectory(texPathname.GetDirectory(), true);

        std::unique_ptr<TextureDescriptor> descriptor(new TextureDescriptor());
        descriptor->compression[eGPUFamily::GPU_MALI].format = PixelFormat::FORMAT_RGB888;
        descriptor->Save(texPathname);

        { // file
            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-texdescriptor",
              "-resave",
              "-file",
              texPathname.GetAbsolutePathname(),
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<TextureDescriptorTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texPathname));
            TEST_VERIFY(descriptor);
        }

        { // folder
            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-texdescriptor",
              "-resave",
              "-folder",
              FilePath(TDTestDetail::testFolderStr).GetAbsolutePathname()
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<TextureDescriptorTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texPathname));
            TEST_VERIFY(descriptor);
        }
        CommandLineModuleTestUtils::ClearTestFolder(TDTestDetail::testFolderStr);
    }

    void CreateEmptyDescriptorWithImage(const DAVA::FilePath& texturePathname)
    {
        using namespace DAVA;

        FilePath pngPathname = FilePath::CreateWithNewExtension(texturePathname, ".png");

        ScopedPtr<Image> image(Image::Create(16u, 16u, PixelFormat::FORMAT_RGBA8888));
        uint8 byte = Random::Instance()->Rand(255);
        Memset(image->data, byte, image->dataSize);

        ImageSystem::Save(pngPathname, image, image->format);
        RETextureDescriptorUtils::CreateOrUpdateDescriptor(pngPathname);
    }

    DAVA_TEST (SetCompressionTest)
    {
        using namespace DAVA;

        CommandLineModuleTestUtils::ClearTestFolder(TDTestDetail::testFolderStr);

        FilePath texPathname = TDTestDetail::testFolderStr + "123/image.tex";
        FileSystem::Instance()->CreateDirectory(texPathname.GetDirectory(), true);

        { // file
            CreateEmptyDescriptorWithImage(texPathname);

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-texdescriptor",
              "-setcompression",
              "-file",
              texPathname.GetAbsolutePathname(),
              "-PowerVR_iOS",
              "A8",
              "-PowerVR_Android",
              "RGBA8888",
              "-adreno",
              "RGBA5551",
              "-mali",
              "RGBA4444",
              "-tegra",
              "RGB888",
              "-dx11",
              "RGB565",
              "-convert",
              "-m",
              "-quality",
              "-2",
              "-f"
            };
            std::unique_ptr<CommandLineModule> tool = std::make_unique<TextureDescriptorTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texPathname));
            if (descriptor)
            {
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_POWERVR_IOS].format == PixelFormat::FORMAT_A8);
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_POWERVR_ANDROID].format == PixelFormat::FORMAT_RGBA8888);
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_ADRENO].format == PixelFormat::FORMAT_RGBA5551);
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_MALI].format == PixelFormat::FORMAT_RGBA4444);
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_TEGRA].format == PixelFormat::FORMAT_RGB888);
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_DX11].format == PixelFormat::FORMAT_RGB565);

                for (uint8 gpu = 0; gpu < eGPUFamily::GPU_DEVICE_COUNT; ++gpu)
                {
                    FilePath compressedPathname = descriptor->CreateMultiMipPathnameForGPU(static_cast<eGPUFamily>(gpu));
                    TEST_VERIFY(FileSystem::Instance()->Exists(compressedPathname));
                }

                TEST_VERIFY(descriptor->GetGenerateMipMaps());
            }
            else
            {
                TEST_VERIFY(false);
            }

            DAVA::FileSystem::Instance()->DeleteDirectoryFiles(texPathname.GetDirectory(), true);
        }

        { // folder
            CreateEmptyDescriptorWithImage(texPathname);

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-texdescriptor",
              "-setcompression",
              "-folder",
              FilePath(TDTestDetail::testFolderStr).GetAbsolutePathname(),
              "-PowerVR_iOS",
              "A8",
              "-PowerVR_Android",
              "RGBA8888",
              "-adreno",
              "RGBA5551",
              "-mali",
              "RGBA4444",
              "-tegra",
              "RGB888",
              "-dx11",
              "RGB565",
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<TextureDescriptorTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texPathname));
            if (descriptor)
            {
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_POWERVR_IOS].format == PixelFormat::FORMAT_A8);
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_POWERVR_ANDROID].format == PixelFormat::FORMAT_RGBA8888);
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_ADRENO].format == PixelFormat::FORMAT_RGBA5551);
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_MALI].format == PixelFormat::FORMAT_RGBA4444);
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_TEGRA].format == PixelFormat::FORMAT_RGB888);
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_DX11].format == PixelFormat::FORMAT_RGB565);

                for (uint8 gpu = 0; gpu < eGPUFamily::GPU_DEVICE_COUNT; ++gpu)
                {
                    FilePath compressedPathname = descriptor->CreateMultiMipPathnameForGPU(static_cast<eGPUFamily>(gpu));
                    TEST_VERIFY(FileSystem::Instance()->Exists(compressedPathname) == false);
                }

                TEST_VERIFY(descriptor->GetGenerateMipMaps() == false);
            }
            else
            {
                TEST_VERIFY(false);
            }

            DAVA::FileSystem::Instance()->DeleteDirectoryFiles(texPathname.GetDirectory(), true);
        }
        CommandLineModuleTestUtils::ClearTestFolder(TDTestDetail::testFolderStr);
    }

    DAVA_TEST (SavePresetTest)
    {
        using namespace DAVA;

        CommandLineModuleTestUtils::CreateTestFolder(TDTestDetail::testFolderStr);

        FilePath texPathname = TDTestDetail::testFolderStr + "123/image.tex";
        FilePath presetPathname = TDTestDetail::testFolderStr + "preset.yaml";

        std::unique_ptr<TextureDescriptor> sourceDescriptor(new TextureDescriptor());
        sourceDescriptor->compression[eGPUFamily::GPU_MALI].format = PixelFormat::FORMAT_RGB888;

        FileSystem::Instance()->CreateDirectory(texPathname.GetDirectory(), true);
        sourceDescriptor->Save(texPathname);

        { // file
            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-texdescriptor",
              "-savepreset",
              "-file",
              texPathname.GetAbsolutePathname(),
              "-preset",
              presetPathname.GetAbsolutePathname()
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<TextureDescriptorTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            ScopedPtr<KeyedArchive> archive(new KeyedArchive());
            TEST_VERIFY(archive->LoadFromYamlFile(presetPathname));

            std::unique_ptr<TextureDescriptor> descriptor(new TextureDescriptor());
            descriptor->DeserializeFromPreset(archive);
            TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_MALI].format == PixelFormat::FORMAT_RGB888);

            FileSystem::Instance()->DeleteFile(presetPathname);
        }

        { // folder
            auto generateLinksFile = [](const FilePath& newFile, const FilePath& link)
            {
                ScopedPtr<File> linksFile(File::Create(newFile, File::WRITE | File::CREATE));
                if (linksFile)
                {
                    linksFile->WriteString(link.GetAbsolutePathname(), false);
                    return true;
                }
                return false;
            };

            FilePath texList = TDTestDetail::testFolderStr + "texList.txt";
            TEST_VERIFY(generateLinksFile(texList, texPathname));

            FilePath presetList = TDTestDetail::testFolderStr + "presetList.txt";
            TEST_VERIFY(generateLinksFile(presetList, presetPathname));

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-texdescriptor",
              "-savepreset",
              "-processfilelist",
              texList.GetAbsolutePathname(),
              "-presetslist",
              presetList.GetAbsolutePathname()
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<TextureDescriptorTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            ScopedPtr<KeyedArchive> archive(new KeyedArchive());
            TEST_VERIFY(archive->LoadFromYamlFile(presetPathname));

            std::unique_ptr<TextureDescriptor> descriptor(new TextureDescriptor());
            descriptor->DeserializeFromPreset(archive);
            TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_MALI].format == PixelFormat::FORMAT_RGB888);

            FileSystem::Instance()->DeleteFile(texPathname);
        }

        CommandLineModuleTestUtils::ClearTestFolder(TDTestDetail::testFolderStr);
    }

    DAVA_TEST (SetPresetTest)
    {
        using namespace DAVA;

        CommandLineModuleTestUtils::ClearTestFolder(TDTestDetail::testFolderStr);

        FilePath imagePathname = TDTestDetail::testFolderStr + "123/image.tga";
        TEST_VERIFY(TDTestDetail::CreateImageFile(imagePathname));

        FilePath texPathname = TDTestDetail::testFolderStr + "123/image.tex";
        std::unique_ptr<TextureDescriptor> sourceDescriptor(new TextureDescriptor());
        sourceDescriptor->compression[eGPUFamily::GPU_MALI].imageFormat = ImageFormat::IMAGE_FORMAT_PVR;
        sourceDescriptor->compression[eGPUFamily::GPU_MALI].format = PixelFormat::FORMAT_RGB888;

        FilePath presetPathname = TDTestDetail::testFolderStr + "preset.yaml";
        TEST_VERIFY(TDTestDetail::SavePreset(sourceDescriptor.get(), presetPathname));

        { // file
            TEST_VERIFY(RETextureDescriptorUtils::CreateOrUpdateDescriptor(imagePathname));

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-texdescriptor",
              "-setpreset",
              "-file",
              texPathname.GetAbsolutePathname(),
              "-preset",
              presetPathname.GetAbsolutePathname(),
              "-convert",
              "-quality",
              "4"
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<TextureDescriptorTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texPathname));
            if (descriptor)
            {
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_MALI].format == PixelFormat::FORMAT_RGB888);

                FilePath compressedPathname = descriptor->CreateMultiMipPathnameForGPU(eGPUFamily::GPU_MALI);
                TEST_VERIFY(FileSystem::Instance()->Exists(compressedPathname));

                FileSystem::Instance()->DeleteFile(compressedPathname);
            }
            else
            {
                TEST_VERIFY(false);
            }
            FileSystem::Instance()->DeleteFile(texPathname);
        }

        { // folder
            TEST_VERIFY(RETextureDescriptorUtils::CreateOrUpdateDescriptor(imagePathname));

            Vector<String> cmdLine =
            {
              "ResourceEditor",
              "-texdescriptor",
              "-setpreset",
              "-folder",
              FilePath(TDTestDetail::testFolderStr).GetAbsolutePathname(),
              "-preset",
              presetPathname.GetAbsolutePathname()
            };

            std::unique_ptr<CommandLineModule> tool = std::make_unique<TextureDescriptorTool>(cmdLine);
            DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

            std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(texPathname));
            if (descriptor)
            {
                TEST_VERIFY(descriptor->compression[eGPUFamily::GPU_MALI].format == PixelFormat::FORMAT_RGB888);
            }
            else
            {
                TEST_VERIFY(false);
            }
            FileSystem::Instance()->DeleteFile(texPathname);
        }

        CommandLineModuleTestUtils::ClearTestFolder(TDTestDetail::testFolderStr);
    }

    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(TArc)
    DECLARE_COVERED_FILES("TextureDescriptorTool.cpp")
    END_FILES_COVERED_BY_TESTS();
}
;
