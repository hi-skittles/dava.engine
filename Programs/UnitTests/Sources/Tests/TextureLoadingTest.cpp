#include "UnitTests/UnitTests.h"
#include "Base/BaseTypes.h"
#include "FileSystem/FileSystem.h"
#include "Render/Image/Image.h"
#include "Render/Image/LibPVRHelper.h"
#include "Render/Texture.h"
#include "Render/TextureDescriptor.h"
#include "Logger/Logger.h"

#include <memory>

using namespace DAVA;

namespace TLTestDetails
{
const String workingFolder("~doc:/TestData/TextureLoadingTest/");
const String texturePathname(workingFolder + "test.tex");

struct TextureData
{
    TextureData(uint32 w = 0, uint32 h = 0, PixelFormat p = PixelFormat::FORMAT_INVALID)
        : width(w)
        , height(h)
        , pixelFormat(p)
    {
    }

    uint32 width = 0;
    uint32 height = 0;
    PixelFormat pixelFormat = PixelFormat::FORMAT_INVALID;
};

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

bool Prepare(const Map<const eGPUFamily, TextureData>& textureData, const Vector<eGPUFamily>& gpuForRealTextures)
{
    FileSystem::eCreateDirectoryResult ret = FileSystem::Instance()->CreateDirectory(workingFolder, true);
    if (ret == FileSystem::DIRECTORY_CANT_CREATE)
        return false;

    std::unique_ptr<TextureDescriptor> descriptor(new TextureDescriptor());
    descriptor->SetGenerateMipmaps(false);

    //setup compression
    for (const std::pair<const eGPUFamily, TextureData>& data : textureData)
    {
        descriptor->compression[data.first].format = data.second.pixelFormat;
        descriptor->compression[data.first].imageFormat = ImageFormat::IMAGE_FORMAT_PVR;
    };

    descriptor->pathname = texturePathname;
    descriptor->Save();

    LibPVRHelper helper;
    for (const eGPUFamily gpu : gpuForRealTextures)
    {
        if (textureData.count(gpu) == 0)
        {
            return false;
        }

        const TextureData& data = textureData.at(gpu);

        ScopedPtr<Image> image(Image::Create(data.width, data.height, data.pixelFormat));
        FilePath savePathname = descriptor->CreateMultiMipPathnameForGPU(gpu);

        eErrorCode writeResult = helper.WriteFile(savePathname, { image }, data.pixelFormat, ImageQuality::DEFAULT_IMAGE_QUALITY);
        if (writeResult != eErrorCode::SUCCESS)
        {
            return false;
        }
    };

    return true;
}

bool Clean()
{
    uint32 count = FileSystem::Instance()->DeleteDirectoryFiles(workingFolder, true);
    return ((count > 0) && FileSystem::Instance()->DeleteDirectory(workingFolder, true));
}
}

DAVA_TESTCLASS (TextureLoadingTest)
{
    DAVA_TEST (Loading)
    {
        const Vector<eGPUFamily> originalGPULoadingOrder = Texture::GetGPULoadingOrder();

        TLTestDetails::ErrorsCounter counter;
        Logger::AddCustomOutput(&counter);
        SCOPE_EXIT
        {
            Logger::RemoveCustomOutput(&counter);
            Texture::SetGPULoadingOrder(originalGPULoadingOrder);
        };

        const Map<const eGPUFamily, TLTestDetails::TextureData> testData =
        {
          { eGPUFamily::GPU_POWERVR_IOS, { 32, 32, PixelFormat::FORMAT_RGBA4444 } },
          { eGPUFamily::GPU_POWERVR_ANDROID, { 64, 64, PixelFormat::FORMAT_RGBA5551 } }
        };
        TEST_VERIFY(TLTestDetails::Prepare(testData, { eGPUFamily::GPU_POWERVR_IOS }));

        { // create and release texture for single GPU
            Texture::SetGPULoadingOrder({ eGPUFamily::GPU_POWERVR_IOS });

            const TLTestDetails::TextureData& textureData = testData.at(eGPUFamily::GPU_POWERVR_IOS);
            ScopedPtr<Texture> texture(Texture::CreateFromFile(TLTestDetails::texturePathname));
            TEST_VERIFY(texture->IsPinkPlaceholder() == false);
            TEST_VERIFY(texture->GetWidth() == textureData.width);
            TEST_VERIFY(texture->GetHeight() == textureData.height);

            TEST_VERIFY(counter.errorsCount == 0);
        }

        { // create and release texture for single GPU
            Texture::SetGPULoadingOrder({ eGPUFamily::GPU_POWERVR_ANDROID, eGPUFamily::GPU_POWERVR_IOS });

            const TLTestDetails::TextureData& textureData = testData.at(eGPUFamily::GPU_POWERVR_IOS);
            ScopedPtr<Texture> texture(Texture::CreateFromFile(TLTestDetails::texturePathname));
            TEST_VERIFY(texture->IsPinkPlaceholder() == false);
            TEST_VERIFY(texture->GetWidth() == textureData.width);
            TEST_VERIFY(texture->GetHeight() == textureData.height);

            TEST_VERIFY(counter.errorsCount == 0);
        }

        TEST_VERIFY(TLTestDetails::Clean());
    }
};
