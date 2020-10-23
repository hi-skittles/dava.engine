#include "UnitTests/UnitTests.h"

#include "Base/BaseTypes.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageConvert.h"
#include "Utils/Random.h"

using namespace DAVA;

DAVA_TESTCLASS (GenerateMipmapsTest)
{
    DAVA_TEST (GenerateTest)
    {
        static Vector<PixelFormat> tests =
        {
          PixelFormat::FORMAT_RGBA8888,
          PixelFormat::FORMAT_RGBA5551,
          //          PixelFormat::FORMAT_RGBA4444, //downscale not implemented
          PixelFormat::FORMAT_RGB888,
          //          PixelFormat::FORMAT_RGB565, //downscale not implemented
          PixelFormat::FORMAT_A8,
          //          PixelFormat::FORMAT_A16, //downscale not implemented
        };

        const uint32 width = 64;
        const uint32 height = 64;
        for (uint32 w = width; w > 0; w >>= 1)
        {
            for (uint32 h = height; h > 0; h >>= 1)
            {
                for (PixelFormat format : tests)
                {
                    Vector<Image*> images;
                    images.push_back(Image::Create(w, h, format));

                    Image* img = images.front();

                    uint8 colorByte = Random::Instance()->Rand(255);
                    Memset(img->GetData(), colorByte, img->GetDataSize());

                    images = img->CreateMipMapsImages(false);
                    SafeRelease(img);

                    for (Image* i : images)
                    {
                        TEST_VERIFY(i->data[0] == colorByte);
                        SafeRelease(i);
                    }
                }
            }
        }
    }
};
