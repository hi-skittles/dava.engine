#include "CommandLine/ImageSplitterTool.h"
#include "CommandLine/Private/CommandLineModuleTestUtils.h"
#include "TArc/Testing/ConsoleModuleTestExecution.h"
#include "TArc/Testing/TArcUnitTests.h"

#include "FileSystem/FileSystem.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"
#include "Utils/Random.h"

#include <memory>

namespace ISTTestDetail
{
const DAVA::String rgbaImagePathname = "~doc:/Test/ImageSplitter/merged.png";
const DAVA::int32 width = 8;
const DAVA::int32 height = 8;
}

DAVA_TARC_TESTCLASS(ImageSplitterToolTest)
{
    DAVA_TEST (SplitTest)
    {
        using namespace DAVA;

        const uint8 r = Random::Instance()->Rand(255);
        const uint8 g = Random::Instance()->Rand(255);
        const uint8 b = Random::Instance()->Rand(255);
        const uint8 a = Random::Instance()->Rand(255);

        CommandLineModuleTestUtils::CreateTestFolder(FilePath(ISTTestDetail::rgbaImagePathname).GetDirectory());

        ScopedPtr<Image> rgbaImage(Image::Create(ISTTestDetail::width, ISTTestDetail::height, PixelFormat::FORMAT_RGBA8888));
        for (uint32 pixelPos = 0; pixelPos < rgbaImage->dataSize; pixelPos += 4)
        {
            rgbaImage->data[pixelPos + 0] = r;
            rgbaImage->data[pixelPos + 1] = g;
            rgbaImage->data[pixelPos + 2] = b;
            rgbaImage->data[pixelPos + 3] = a;
        }

        eErrorCode saved = ImageSystem::Save(ISTTestDetail::rgbaImagePathname, rgbaImage);
        TEST_VERIFY(saved == eErrorCode::SUCCESS);

        Vector<String> cmdLine =
        {
          "ResourceEditor",
          "-imagesplitter",
          "-split",
          "-file",
          FilePath(ISTTestDetail::rgbaImagePathname).GetAbsolutePathname()
        };

        std::unique_ptr<CommandLineModule> tool = std::make_unique<ImageSplitterTool>(cmdLine);
        DAVA::TArc::ConsoleModuleTestExecution::ExecuteModule(tool.get());

        auto testChannel = [](const String& channelName, uint8 channelValue)
        {
            FilePath imagePathname(ISTTestDetail::rgbaImagePathname);
            imagePathname.ReplaceFilename(channelName);

            ScopedPtr<Image> image(ImageSystem::LoadSingleMip(imagePathname));
            if (image)
            {
                if (image->format != PixelFormat::FORMAT_A8)
                {
                    return false;
                }

                for (uint32 pixel = 0; pixel < image->dataSize; ++pixel)
                {
                    if (image->data[pixel] != channelValue)
                        return false;
                }
            }
            else
            {
                return false;
            }

            return true;
        };

        TEST_VERIFY(testChannel("r.png", r));
        TEST_VERIFY(testChannel("g.png", g));
        TEST_VERIFY(testChannel("b.png", b));
        TEST_VERIFY(testChannel("a.png", a));

        CommandLineModuleTestUtils::ClearTestFolder(FilePath(ISTTestDetail::rgbaImagePathname).GetDirectory());
    }

    DAVA_TEST (MergeTest)
    {
        using namespace DAVA;

        const uint8 r = Random::Instance()->Rand(255);
        const uint8 g = Random::Instance()->Rand(255);
        const uint8 b = Random::Instance()->Rand(255);
        const uint8 a = Random::Instance()->Rand(255);

        CommandLineModuleTestUtils::CreateTestFolder(FilePath(ISTTestDetail::rgbaImagePathname).GetDirectory());

        auto createChannel = [](const String& channelName, uint8 channelValue)
        {
            FilePath imagePathname(ISTTestDetail::rgbaImagePathname);
            imagePathname.ReplaceFilename(channelName);

            ScopedPtr<Image> channelImage(Image::Create(ISTTestDetail::width, ISTTestDetail::height, PixelFormat::FORMAT_A8));
            if (channelImage)
            {
                Memset(channelImage->data, channelValue, channelImage->dataSize);

                eErrorCode saved = ImageSystem::Save(imagePathname, channelImage, channelImage->format);
                return (saved == eErrorCode::SUCCESS);
            }
            return false;
        };

        TEST_VERIFY(createChannel("r.png", r));
        TEST_VERIFY(createChannel("g.png", g));
        TEST_VERIFY(createChannel("b.png", b));
        TEST_VERIFY(createChannel("a.png", a));

        Vector<String> cmdLine =
        {
          "ResourceEditor",
          "-imagesplitter",
          "-merge",
          "-folder",
          FilePath(ISTTestDetail::rgbaImagePathname).GetDirectory().GetAbsolutePathname()
        };

        std::unique_ptr<CommandLineModule> tool = std::make_unique<ImageSplitterTool>(cmdLine);
        DAVA::TArc::ConsoleModuleTestExecution::ExecuteModule(tool.get());

        ScopedPtr<Image> rgbaImage(ImageSystem::LoadSingleMip(ISTTestDetail::rgbaImagePathname));
        if (rgbaImage)
        {
            for (uint32 pixelPos = 0; pixelPos < rgbaImage->dataSize; pixelPos += 4)
            {
                bool imageDataIsCorrect = (rgbaImage->data[pixelPos + 0] == r) && (rgbaImage->data[pixelPos + 1] == g) && (rgbaImage->data[pixelPos + 2] == b) && (rgbaImage->data[pixelPos + 3] == a);
                if (imageDataIsCorrect == false)
                {
                    TEST_VERIFY(false);
                    break;
                }
            }
        }
        else
        {
            TEST_VERIFY(false);
        }

        CommandLineModuleTestUtils::ClearTestFolder(FilePath(ISTTestDetail::rgbaImagePathname).GetDirectory());
    }

    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(TArc)
    DECLARE_COVERED_FILES("ImageSplitterTool.cpp")
    END_FILES_COVERED_BY_TESTS();
};
