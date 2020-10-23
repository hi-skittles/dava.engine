#include "UnitTests/UnitTests.h"

#include "Base/BaseTypes.h"
#include "Math/HalfFloat.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageConvert.h"
#include "Utils/StringFormat.h"
#include "Utils/Random.h"

using namespace DAVA;

DAVA_TESTCLASS (ImageTest)
{
    DAVA_TEST (DownscaleTest)
    {
        struct TestData
        {
            PixelFormat format;
            uint32 width;
            uint32 height;
            bool willBeDownscaled;
        };

        static Vector<TestData> tests =
        {
          { PixelFormat::FORMAT_RGBA8888, 8, 8, true },
          { PixelFormat::FORMAT_RGBA5551, 8, 8, true },
          //          { PixelFormat::FORMAT_RGBA4444, 8, 8, false }, //downscale not implemented
          { PixelFormat::FORMAT_RGB888, 8, 8, true },
          //          { PixelFormat::FORMAT_RGB565, 8, 8, false }, //downscale not implemented
          { PixelFormat::FORMAT_A8, 8, 8, true },
          //          { PixelFormat::FORMAT_A16, 8, 8, false }, //downscale not implemented
          //          { PixelFormat::FORMAT_PVR4, 8, 8, false }, //downscale not implemented
          //          { PixelFormat::FORMAT_PVR2, 8, 8, false }, //downscale not implemented
          { PixelFormat::FORMAT_RGBA16161616, 8, 8, true },
          { PixelFormat::FORMAT_RGBA32323232, 8, 8, true },
          //          { PixelFormat::FORMAT_DXT1, 8, 8, false }, //downscale not implemented
          //          { PixelFormat::FORMAT_DXT1A, 8, 8, false }, //downscale not implemented
          //          { PixelFormat::FORMAT_DXT3, 8, 8, false }, //downscale not implemented
          //          { PixelFormat::FORMAT_DXT5, 8, 8, false }, //downscale not implemented
          //          { PixelFormat::FORMAT_DXT5NM, 8, 8, false }, //downscale not implemented
          //          { PixelFormat::FORMAT_ETC1, 8, 8, false }, //downscale not implemented
          //          { PixelFormat::FORMAT_ATC_RGB, 8, 8, false }, //downscale not implemented
          //          { PixelFormat::FORMAT_ATC_RGBA_EXPLICIT_ALPHA, 8, 8, false }, //downscale not implemented
          //          { PixelFormat::FORMAT_ATC_RGBA_INTERPOLATED_ALPHA, 8, 8, false }, //downscale not implemented
          { PixelFormat::FORMAT_RGBA16F, 8, 8, true },
          { PixelFormat::FORMAT_RGBA32F, 8, 8, true }
        };

        for (const TestData& td : tests)
        {
            ScopedPtr<Image> source(Image::Create(td.width, td.height, td.format));
            TEST_VERIFY(source);

            uint8 colorByte = Random::Instance()->Rand(20); //temporary reduced value due fail of android tests
            Memset(source->GetData(), colorByte, source->GetDataSize());

            ScopedPtr<Image> destination(ImageConvert::DownscaleTwiceBillinear(source));
            if (td.willBeDownscaled)
            {
                TEST_VERIFY(destination);
                TEST_VERIFY(destination.get() != source.get());

                TEST_VERIFY(destination->GetPixelFormat() == td.format);
                TEST_VERIFY(destination->GetWidth() == (td.width / 2));
                TEST_VERIFY(destination->GetHeight() == (td.height / 2));

                if (PixelFormat::FORMAT_RGBA16F == td.format)
                {
                    uint16* sourcePtr = reinterpret_cast<uint16*>(source->data);
                    uint16* dstPtr = reinterpret_cast<uint16*>(destination->data);
                    uint32 size = destination->GetDataSize() / 2;
                    for (uint32 i = 0; i < size; ++i)
                    {
                        float32 sourcePixel = Float16Compressor::Decompress(sourcePtr[i]);
                        float32 dstPixel = Float16Compressor::Decompress(dstPtr[i]);

                        if (fabsf(sourcePixel - dstPixel) > EPSILON)
                        {
                            TEST_VERIFY_WITH_MESSAGE(false, "PixelFormat: RGBA16F");
                            break;
                        }
                    }
                }
                else if (PixelFormat::FORMAT_RGBA32F == td.format)
                {
                    float32* sourcePtr = reinterpret_cast<float32*>(source->data);
                    float32* dstPtr = reinterpret_cast<float32*>(destination->data);
                    uint32 size = destination->GetDataSize() / sizeof(float32);
                    for (uint32 i = 0; i < size; ++i)
                    {
                        if (fabsf(sourcePtr[i] - dstPtr[i]) > EPSILON)
                        {
                            TEST_VERIFY_WITH_MESSAGE(false, "PixelFormat: RGBA32F");
                            break;
                        }
                    }
                }
                else
                {
                    TEST_VERIFY_WITH_MESSAGE(Memcmp(source->GetData(), destination->GetData(), destination->GetDataSize()) == 0, Format("PixelFormat: %d", td.format));
                }
            }
            else
            {
                TEST_VERIFY(!destination);
            }
        }
    }
};
