#include "Base/GlobalEnum.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageConvert.h"
#include "Render/Image/ImageSystem.h"
#include "Render/PixelFormatDescriptor.h"
#include "Render/RHI/rhi_Public.h"
#include "UnitTests/UnitTests.h"
#include "Utils/StringFormat.h"

#include "Infrastructure/TextureUtils.h"

using namespace DAVA;

const float32 MAX_DIFFERENCE = 2.f; // in percents

#ifndef __DAVAENGINE_WIN_UAP__

DAVA_TESTCLASS (FormatsTest)
{
    DAVA_TEST (TestJpeg)
    {
        Vector<PixelFormat> suportedFormats;
        suportedFormats.push_back(FORMAT_A8);
        suportedFormats.push_back(FORMAT_RGB888);

        for (PixelFormat requestedFormat : suportedFormats)
        {
            const String formatName = GlobalEnumMap<DAVA::PixelFormat>::Instance()->ToString(requestedFormat);
            const DAVA::FilePath compressedPathname(DAVA::Format("~res:/TestData/FormatsTest/jpeg/%s.dat", formatName.c_str()));
            TestImageInfo(compressedPathname, requestedFormat);
        }
    }

    DAVA_TEST (TestPng)
    {
        Vector<PixelFormat> suportedFormats;
        suportedFormats.push_back(FORMAT_A8);
        suportedFormats.push_back(FORMAT_A16);
        suportedFormats.push_back(FORMAT_RGBA8888);

        for (PixelFormat requestedFormat : suportedFormats)
        {
            const String formatName = GlobalEnumMap<DAVA::PixelFormat>::Instance()->ToString(requestedFormat);
            const DAVA::FilePath compressedPathname(DAVA::Format("~res:/TestData/FormatsTest/png/%s.dat", formatName.c_str()));
            TestImageInfo(compressedPathname, requestedFormat);
        }
    }

    DAVA_TEST (TestPvr)
    {
        Vector<PixelFormat> suportedFormats;
        suportedFormats.push_back(FORMAT_RGBA8888);
        suportedFormats.push_back(FORMAT_RGBA5551);
        suportedFormats.push_back(FORMAT_RGBA4444);
        if (rhi::TextureFormatSupported(rhi::TEXTURE_FORMAT_R8G8B8))
            suportedFormats.push_back(FORMAT_RGB888);
        suportedFormats.push_back(FORMAT_RGB565);
        suportedFormats.push_back(FORMAT_A8);
        suportedFormats.push_back(FORMAT_PVR2);
        suportedFormats.push_back(FORMAT_PVR4);
        suportedFormats.push_back(FORMAT_ETC1);
        suportedFormats.push_back(FORMAT_RGBA16F);
        suportedFormats.push_back(FORMAT_RGBA32F);

        for (PixelFormat requestedFormat : suportedFormats)
        {
            const String formatName = GlobalEnumMap<DAVA::PixelFormat>::Instance()->ToString(requestedFormat);
            const DAVA::FilePath pngPathname(DAVA::Format("~res:/TestData/FormatsTest/pvr/%s.png", formatName.c_str()));
            const DAVA::FilePath compressedPathname = DAVA::FilePath::CreateWithNewExtension(pngPathname, ".pvr");
            TestImageInfo(compressedPathname, requestedFormat);

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WIN32__)
            const DAVA::PixelFormatDescriptor& descriptor = DAVA::PixelFormatDescriptor::GetPixelFormatDescriptor(requestedFormat);
            if (descriptor.isHardwareSupported)
                continue;

            ScopedPtr<Image> pngImage(ImageSystem::LoadSingleMip(pngPathname, 0));
            TEST_VERIFY(pngImage);

            ScopedPtr<Image> compressedImage(ImageSystem::LoadSingleMip(compressedPathname, 0));
            TEST_VERIFY(compressedImage);

            if (pngImage && compressedImage)
            {
                ScopedPtr<Image> decompressedImage(Image::Create(compressedImage->width, compressedImage->height, FORMAT_RGBA8888));
                TEST_VERIFY(ImageConvert::CanConvertFromTo(compressedImage->format, FORMAT_RGBA8888) == true);
                TEST_VERIFY(ImageConvert::ConvertImage(compressedImage, decompressedImage) == true);
                const TextureUtils::CompareResult cmpRes = TextureUtils::CompareImages(pngImage, decompressedImage, FORMAT_RGBA8888);

                float32 differencePersentage = (cmpRes.difference / (cmpRes.bytesCount * 256.f)) * 100.f;
                TEST_VERIFY_WITH_MESSAGE(differencePersentage <= MAX_DIFFERENCE, Format("Difference=%f%%, Coincidence=%f%%", differencePersentage, 100.f - differencePersentage));
            }
#endif //#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WIN32__)
        }
    }
    
#if !defined(__DAVAENGINE_IPHONE__)
    DAVA_TEST (TestDds)
    {
        Vector<PixelFormat> suportedFormats;
        suportedFormats.push_back(FORMAT_DXT1);
        suportedFormats.push_back(FORMAT_DXT1A);
        suportedFormats.push_back(FORMAT_DXT3);
        suportedFormats.push_back(FORMAT_DXT5);
        suportedFormats.push_back(FORMAT_DXT5NM);
        suportedFormats.push_back(FORMAT_ATC_RGB);
        suportedFormats.push_back(FORMAT_ATC_RGBA_EXPLICIT_ALPHA);
        suportedFormats.push_back(FORMAT_ATC_RGBA_INTERPOLATED_ALPHA);
        suportedFormats.push_back(FORMAT_RGBA16F);
        suportedFormats.push_back(FORMAT_RGBA32F);

        for (PixelFormat requestedFormat : suportedFormats)
        {
            const String formatName = GlobalEnumMap<DAVA::PixelFormat>::Instance()->ToString(requestedFormat);
            const DAVA::FilePath pngPathname(DAVA::Format("~res:/TestData/FormatsTest/dds/%s.png", formatName.c_str()));
            const DAVA::FilePath compressedPathname = DAVA::FilePath::CreateWithNewExtension(pngPathname, ".dds");
            TestImageInfo(compressedPathname, requestedFormat);

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WIN32__)
            if (requestedFormat == FORMAT_ATC_RGBA_INTERPOLATED_ALPHA)
            {
                continue;
            }

            const DAVA::PixelFormatDescriptor& descriptor = DAVA::PixelFormatDescriptor::GetPixelFormatDescriptor(requestedFormat);
            if (descriptor.isHardwareSupported)
                continue;

            ScopedPtr<Image> pngImage(ImageSystem::LoadSingleMip(pngPathname, 0));
            TEST_VERIFY(pngImage);

            ScopedPtr<Image> compressedImage(ImageSystem::LoadSingleMip(compressedPathname, 0));
            TEST_VERIFY(compressedImage);

            if (pngImage && compressedImage)
            {
                ScopedPtr<Image> decompressedImage(Image::Create(compressedImage->width, compressedImage->height, FORMAT_RGBA8888));
                TEST_VERIFY(ImageConvert::CanConvertFromTo(compressedImage->format, FORMAT_RGBA8888) == true);
                TEST_VERIFY(ImageConvert::ConvertImage(compressedImage, decompressedImage) == true);
                const TextureUtils::CompareResult cmpRes = TextureUtils::CompareImages(pngImage, decompressedImage, FORMAT_RGBA8888);

                float32 differencePersentage = (cmpRes.difference / (cmpRes.bytesCount * 256.f)) * 100.f;
                TEST_VERIFY_WITH_MESSAGE(differencePersentage <= MAX_DIFFERENCE, Format("Difference=%f%%, Coincidence=%f%%", differencePersentage, 100.f - differencePersentage));
            }
#endif //#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WIN32__)
        }
    }
#endif //#if !defined(__DAVAENGINE_IPHONE__)

    DAVA_TEST (TestWebP)
    {
        Vector<PixelFormat> suportedFormats;
        suportedFormats.push_back(FORMAT_RGB888);
        suportedFormats.push_back(FORMAT_RGBA8888);

        for (PixelFormat requestedFormat : suportedFormats)
        {
            const String formatName = GlobalEnumMap<DAVA::PixelFormat>::Instance()->ToString(requestedFormat);
            const DAVA::FilePath compressedPathname(DAVA::Format("~res:/TestData/FormatsTest/webp/%s.dat", formatName.c_str()));
            TestImageInfo(compressedPathname, requestedFormat);
        }
    }

    DAVA_TEST (TestHDR)
    {
        Vector<PixelFormat> suportedFormats;
        suportedFormats.push_back(FORMAT_RGBA32F);

        for (PixelFormat requestedFormat : suportedFormats)
        {
            const String formatName = GlobalEnumMap<DAVA::PixelFormat>::Instance()->ToString(requestedFormat);
            const DAVA::FilePath compressedPathname(DAVA::Format("~res:/TestData/FormatsTest/hdr/%s.dat", formatName.c_str()));
            TestImageInfo(compressedPathname, requestedFormat);
        }
    }

    void TestImageInfo(const DAVA::FilePath& fileName, DAVA::PixelFormat& requestedFormat)
    {
        // NOTE: if file is generated in DXT1A format then lib returned new file in DXT1
        switch (requestedFormat)
        {
        case FORMAT_DXT1A:
            requestedFormat = FORMAT_DXT1;
            break;
        default:
            break;
        }

        ImageInfo info = ImageSystem::GetImageInfo(fileName);
        TEST_VERIFY(info.format == requestedFormat);
        TEST_VERIFY(info.width == 256);
        TEST_VERIFY(info.height == 256);
    }
};

#else

__DAVAENGINE_WIN_UAP_INCOMPLETE_IMPLEMENTATION__MARKER__

#endif //  !__DAVAENGINE_WIN_UAP__
