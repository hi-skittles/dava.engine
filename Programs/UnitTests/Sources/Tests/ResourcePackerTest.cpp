#include <DAVAEngine.h>
#include <UnitTests/UnitTests.h>

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)

#include <TexturePacker/ResourcePacker2D.h>

#include <Engine/EngineContext.h>
#include <FileSystem/FileSystem.h>
#include <Render/PixelFormatDescriptor.h>

DAVA_TESTCLASS (ResourcePackerTest)
{
    const DAVA::FilePath resourcesDir = "~res:/TestData/ResourcePackerTest/SourceFiles/"; // this folder contains 5 files: air.png, air.psd, arrow_tut.psd, eye_tut.psd, target_tut.psd
    const DAVA::FilePath rootDir = "~doc:/TestData/ResourcePackerTest/";
    const DAVA::FilePath inputDir = rootDir + "Input/";
    const DAVA::FilePath outputDir = rootDir + "Output/";
    const DAVA::Vector<DAVA::String> psdBaseNames = { "air", "arrow_tut", "eye_tut", "target_tut" };

    ResourcePackerTest()
    {
    }

    void ClearWorkingFolders()
    {
        DAVA::GetEngineContext()->fileSystem->DeleteDirectory(rootDir, true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(rootDir) == false);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CreateDirectory(inputDir, true) != DAVA::FileSystem::DIRECTORY_CANT_CREATE);
    }

    void CopyPsdSources()
    {
        for (const DAVA::String& basename : psdBaseNames)
        {
            DAVA::String fullName = basename + ".psd";
            TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CopyFile(resourcesDir + fullName, inputDir + fullName) == true);
        }
    }

    DAVA_TEST (GpuTest)
    {
        using namespace DAVA;

        ClearWorkingFolders();
        CopyPsdSources();

        struct GpuParams
        {
            eGPUFamily gpu = eGPUFamily::GPU_ORIGIN;
            PixelFormat pixelFormat = FORMAT_RGBA8888;
            ImageFormat imageFormat = IMAGE_FORMAT_PNG;
        };

        Vector<GpuParams> gpuParams = {
            { eGPUFamily::GPU_POWERVR_IOS, PixelFormat::FORMAT_RGBA8888, ImageFormat::IMAGE_FORMAT_PVR },
            { eGPUFamily::GPU_POWERVR_ANDROID, PixelFormat::FORMAT_RGBA8888, ImageFormat::IMAGE_FORMAT_WEBP },
            { eGPUFamily::GPU_TEGRA, PixelFormat::FORMAT_RGB888, ImageFormat::IMAGE_FORMAT_JPEG },
            { eGPUFamily::GPU_MALI, PixelFormat::FORMAT_RGBA8888, ImageFormat::IMAGE_FORMAT_TGA },
            { eGPUFamily::GPU_ADRENO, PixelFormat::FORMAT_ATC_RGB, ImageFormat::IMAGE_FORMAT_DDS },
            { eGPUFamily::GPU_DX11, PixelFormat::FORMAT_DXT1, ImageFormat::IMAGE_FORMAT_DDS },
            { eGPUFamily::GPU_ORIGIN, PixelFormat::FORMAT_RGBA8888, ImageFormat::IMAGE_FORMAT_PNG }
        };

        Vector<eGPUFamily> requestedGPUs;
        {
            ScopedPtr<File> flagsFile(File::Create(inputDir + "flags.txt", File::CREATE | File::WRITE));
            for (const GpuParams& params : gpuParams)
            {
                const String& gpuName = GPUFamilyDescriptor::GetGPUName(params.gpu);
                const char* pixelFormatString = PixelFormatDescriptor::GetPixelFormatString(params.pixelFormat);
                const String& imageFormatString = ImageSystem::GetImageFormatInterface(params.imageFormat)->GetName();
                String flagString = Format("--%s %s %s ", gpuName.c_str(), pixelFormatString, imageFormatString.c_str());
                flagsFile->WriteNonTerminatedString(flagString);
                requestedGPUs.push_back(params.gpu);
            }
        }

        ResourcePacker2D packer;
        packer.InitFolders(inputDir, outputDir);
        packer.PackResources(requestedGPUs);

        TEST_VERIFY(packer.GetErrors().empty() == true);

        for (const String& name : psdBaseNames)
        {
            String fullName = name + ".txt";
            TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(outputDir + fullName) == true);
        }

        for (const GpuParams& params : gpuParams)
        {
            String expectedSheetName = "texture0" + GPUFamilyDescriptor::GetGPUPrefix(params.gpu) + ImageSystem::GetDefaultExtension(params.imageFormat);
            TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(outputDir + expectedSheetName) == true);
        }
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(outputDir + "texture0.tex") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(outputDir + "flags.txt") == false);
    };

    DAVA_TEST (WrongGpuParamsTest)
    {
        using namespace DAVA;

        ClearWorkingFolders();
        CopyPsdSources();

        {
            ScopedPtr<File> flagsFile(File::Create(inputDir + "flags.txt", File::CREATE | File::WRITE));
            flagsFile->WriteLine("--dx11 RGBA8888 dds");
        }

        ResourcePacker2D packer;
        packer.InitFolders(inputDir, outputDir);
        packer.PackResources({ eGPUFamily::GPU_DX11 });

        TEST_VERIFY(packer.GetErrors().empty() == false); // should contain error about incorrect flags for requested GPU
    }

    DAVA_TEST (NoFlagsForGpuTest)
    {
        ClearWorkingFolders();
        CopyPsdSources();

        DAVA::ResourcePacker2D packer;
        packer.InitFolders(inputDir, outputDir);
        packer.PackResources({ DAVA::eGPUFamily::GPU_DX11 });

        TEST_VERIFY(packer.GetErrors().empty() == false); // should contain error about absence of gpu key in flags.txt
    };

    DAVA_TEST (SourcePngTest)
    {
        using namespace DAVA;

        ClearWorkingFolders();
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CopyFile(resourcesDir + "air.png", inputDir + "air.png") == true);

        ResourcePacker2D packer;
        packer.InitFolders(inputDir, outputDir);
        packer.PackResources({ eGPUFamily::GPU_ORIGIN });

        TEST_VERIFY(packer.GetErrors().empty() == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(outputDir + "air.txt") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(outputDir + "texture0.png") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(outputDir + "texture0.tex") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(outputDir + "flags.txt") == false);
    }

    DAVA_TEST (TagsTest)
    {
        using namespace DAVA;
        ClearWorkingFolders();

        {
            TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CopyFile(resourcesDir + "arrow_tut.psd", inputDir + "arrow_tut.psd") == true);
            TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CopyFile(resourcesDir + "eye_tut.psd", inputDir + "eye_tut.psd") == true);

            ResourcePacker2D packer;
            packer.InitFolders(inputDir, outputDir);
            packer.PackResources({ eGPUFamily::GPU_ORIGIN });
            TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(outputDir + "texture0.png") == true);
        }

        FilePath tagsOutputDir = rootDir + "OutputWithTags/";
        DAVA::GetEngineContext()->fileSystem->DeleteDirectory(inputDir, true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(inputDir) == false);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CreateDirectory(inputDir, true) != FileSystem::DIRECTORY_CANT_CREATE);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CopyFile(resourcesDir + "air.psd", inputDir + "arrow_tut.psd") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CopyFile(resourcesDir + "arrow_tut.psd", inputDir + "arrow_tut.china.psd") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CopyFile(resourcesDir + "eye_tut.psd", inputDir + "eye_tut.psd") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CopyFile(resourcesDir + "target_tut.psd", inputDir + "target_tut.japan.psd") == true);

        ResourcePacker2D packer;
        packer.InitFolders(inputDir, tagsOutputDir);
        packer.SetAllTags({ ".china", ".japan" });
        packer.SetTag(".china");
        packer.PackResources({ eGPUFamily::GPU_ORIGIN });

        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(tagsOutputDir + "texture0.png") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(tagsOutputDir + "texture0.tex") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(tagsOutputDir + "arrow_tut.txt") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(tagsOutputDir + "eye_tut.txt") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(tagsOutputDir + "arrow_tut.china.txt") == false);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(tagsOutputDir + "target_tut.txt") == false);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->Exists(tagsOutputDir + "target_tut.japan.txt") == false);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CompareBinaryFiles(outputDir + "texture0.png", tagsOutputDir + "texture0.png") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CompareBinaryFiles(outputDir + "texture0.tex", tagsOutputDir + "texture0.tex") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CompareTextFiles(outputDir + "arrow_tut.txt", tagsOutputDir + "arrow_tut.txt") == true);
        TEST_VERIFY(DAVA::GetEngineContext()->fileSystem->CompareTextFiles(outputDir + "eye_tut.txt", tagsOutputDir + "eye_tut.txt") == true);
    };

    DAVA_TEST (MissingTagTest)
    {
        using namespace DAVA;

        ClearWorkingFolders();
        CopyPsdSources();

        ResourcePacker2D packer;
        packer.InitFolders(inputDir, outputDir);
        packer.SetAllTags({ ".japan" });
        packer.SetTag(".china");
        packer.PackResources({ eGPUFamily::GPU_ORIGIN });

        TEST_VERIFY(packer.GetErrors().empty() == false); // should contain error about absence of ".china" tag in allTags
    };
};

#endif
