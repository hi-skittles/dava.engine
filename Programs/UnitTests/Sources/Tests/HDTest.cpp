#include "UnitTests/UnitTests.h"
#include "Base/BaseTypes.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageSystem.h"
#include "Render/Texture.h"
#include "Render/TextureDescriptor.h"
#include "Render/Image/ImageSystem.h"
#include "Logger/Logger.h"

#include <memory>

using namespace DAVA;

namespace HDTestDetail
{
class ErrorsCounter : public LoggerOutput
{
public:
    void Output(Logger::eLogLevel ll, const char8* text) override
    {
        if (ll == Logger::LEVEL_ERROR)
        {
            ++errorsCount;
        }
    }
    uint32 errorsCount = 0;
};

void TestTexture(const FilePath& pathname, uint32 expectedSize)
{
    ScopedPtr<Texture> texture(Texture::CreateFromFile(pathname));
    TEST_VERIFY(texture);
    if (!texture)
    {
        return;
    }
    TEST_VERIFY(texture->GetWidth() == texture->GetHeight());
    TEST_VERIFY(texture->GetWidth() == expectedSize);
}

bool LoadImagesAsInTexture(const FilePath& pathname, eGPUFamily gpu, uint32 expectedSize, uint32 baseMipMap, bool expectSingleMipmapFile)
{
    //load descriptor from file
    std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(pathname));
    if (!descriptor)
    {
        Logger::Error("Can't create TextureDescriptor from %s", pathname.GetStringValue().c_str());
        return false;
    }

    //setup initial params for loading
    ImageSystem::LoadingParams params;
    params.baseMipmap = baseMipMap;
    params.firstMipmapIndex = 0;
    params.minimalWidth = Texture::MINIMAL_WIDTH;
    params.minimalHeight = Texture::MINIMAL_HEIGHT;

    Vector<Image*> images;
    SCOPE_EXIT
    {
        for (Image* img : images)
        {
            SafeRelease(img);
        }
        images.clear();
    };

    //load images
    Vector<FilePath> singleMipFiles;
    bool hasSingleMipFiles = descriptor->CreateSingleMipPathnamesForGPU(gpu, singleMipFiles);
    if (hasSingleMipFiles)
    {
        uint32 singleMipFilesCount = static_cast<uint32>(singleMipFiles.size());
        TEST_VERIFY(singleMipFilesCount > 0);

        for (uint32 index = baseMipMap; index < singleMipFilesCount; ++index)
        {
            params.baseMipmap = 0;
            params.firstMipmapIndex = static_cast<uint32>(images.size());
            eErrorCode loadingResult = ImageSystem::Load(singleMipFiles[index], images, params);
            if (expectSingleMipmapFile)
            {
                TEST_VERIFY(loadingResult == eErrorCode::SUCCESS);
            }
            else
            {
                TEST_VERIFY(loadingResult == eErrorCode::ERROR_FILE_NOTFOUND);
            }
        }

        params.baseMipmap = Max(static_cast<int32>(baseMipMap) - static_cast<int32>(singleMipFilesCount), 0);
        params.firstMipmapIndex += static_cast<uint32>(images.size());

        if (expectSingleMipmapFile)
        {
            TEST_VERIFY(images.size() == 1);
        }
        else
        {
            TEST_VERIFY(images.empty());
        }
    }

    FilePath multipleMipPathname = descriptor->CreateMultiMipPathnameForGPU(gpu);
    TEST_VERIFY(ImageSystem::Load(multipleMipPathname, images, params) == eErrorCode::SUCCESS);

    if (images.empty())
    {
        Logger::Error("Can't load images");
        return false;
    }

    int32 index = 0;
    for (Image* img : images)
    {
        TEST_VERIFY(img->GetWidth() == img->GetHeight());
        TEST_VERIFY(img->GetWidth() == (expectedSize >> index));

        TEST_VERIFY(img->mipmapLevel == index);
        ++index;
    }

    return true;
}
}

DAVA_TESTCLASS (HDTest)
{
    DAVA_TEST (WithHDFile)
    {
        HDTestDetail::ErrorsCounter counter;
        Logger::AddCustomOutput(&counter);
        SCOPE_EXIT
        {
            Logger::RemoveCustomOutput(&counter);
        };

        HDTestDetail::TestTexture("~res:/TestData/HDTest/WithZeroMip/test.tex", 64);
        TEST_VERIFY(HDTestDetail::LoadImagesAsInTexture("~res:/TestData/HDTest/WithZeroMip/test.tex", eGPUFamily::GPU_POWERVR_IOS, 64, 0, true));

        TEST_VERIFY(counter.errorsCount == 0);
    }

    DAVA_TEST (WithoutHDFile)
    {
        HDTestDetail::ErrorsCounter counter;
        Logger::AddCustomOutput(&counter);
        SCOPE_EXIT
        {
            Logger::RemoveCustomOutput(&counter);
        };

        HDTestDetail::TestTexture("~res:/TestData/HDTest/WithoutZeroMip/test.tex", 32);
        TEST_VERIFY(HDTestDetail::LoadImagesAsInTexture("~res:/TestData/HDTest/WithoutZeroMip/test.tex", eGPUFamily::GPU_POWERVR_IOS, 32, 0, false));

        TEST_VERIFY(counter.errorsCount == 0);
    }
};
