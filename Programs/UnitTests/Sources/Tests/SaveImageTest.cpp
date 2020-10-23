#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

#include "Infrastructure/TextureUtils.h"

using namespace DAVA;

const float32 LOSSY_ALLOWED_DIFF = 2.f; //in percents
const float32 LOSSLESS_ALLOWED_DIFF = 0.f; //in percents

DAVA_TESTCLASS (SaveImageTest)
{
    Image* imageRGBA8888 = nullptr;
    Image* imageRGB888 = nullptr;
    Image* imageA8 = nullptr;

    SaveImageTest()
    {
        imageRGBA8888 = Create8888Image();
        imageRGB888 = Create888Image();
        imageA8 = CreateA8Image();
    }
    ~SaveImageTest()
    {
        SafeRelease(imageRGBA8888);
        SafeRelease(imageRGB888);
        SafeRelease(imageA8);
    }

    DAVA_TEST (PngTest)
    {
        SaveLoadCheck(imageRGBA8888, "testRGBA8888.png", LOSSLESS_ALLOWED_DIFF);
        //SaveLoadCheck(imageRGB888, "testRGB888.png", LOSSELESS_ALLOWED_DIFF); -- RGB888 is not supported by PNG
        SaveLoadCheck(imageA8, "testA8.png", LOSSLESS_ALLOWED_DIFF);
    }

    DAVA_TEST (JpegTest)
    {
        //SaveLoadCheck(imageRGBA8888, "testRGBA8888.jpg", LOSSY_ALLOWED_DIFF); -- RGBA8888 is not supported for JPEG
        SaveLoadCheck(imageRGB888, "testRGB888.jpg", LOSSY_ALLOWED_DIFF);
        SaveLoadCheck(imageA8, "testA8.jpg", LOSSY_ALLOWED_DIFF);
    }

    DAVA_TEST (TgaTest)
    {
        SaveLoadCheck(imageRGBA8888, "testRGBA8888.tga", LOSSLESS_ALLOWED_DIFF);
        SaveLoadCheck(imageRGB888, "testRGB888.tga", LOSSLESS_ALLOWED_DIFF);
        SaveLoadCheck(imageA8, "testA8.tga", LOSSLESS_ALLOWED_DIFF);
    }

    DAVA_TEST (WebPTest)
    {
        SaveLoadCheck(imageRGB888, "testRGB888.webp", LOSSY_ALLOWED_DIFF);
        SaveLoadCheck(imageRGBA8888, "testRGBA8888.webp", LOSSY_ALLOWED_DIFF);
    }

    void SaveLoadCheck(const Image* inImage, const String& filename, float32 diffThreshold)
    {
        FilePath path = FilePath::FilepathInDocuments(filename);

        DAVA::Vector<DAVA::Image*> imgSet;

        TEST_VERIFY(inImage->Save(path));

        TEST_VERIFY(DAVA::ImageSystem::Load(path, imgSet) == DAVA::eErrorCode::SUCCESS);
        TEST_VERIFY(imgSet[0]->dataSize == inImage->dataSize);

        const TextureUtils::CompareResult cmpRes = TextureUtils::CompareImages(inImage, imgSet[0], inImage->format);
        float32 differencePersentage = (cmpRes.difference / (cmpRes.bytesCount * 256.f)) * 100.f;
        TEST_VERIFY(differencePersentage <= diffThreshold);

        for (auto img : imgSet)
        {
            img->Release();
        }
    }

    Image* Create8888Image() const
    {
        uint32 size = 512;
        Image* img = Image::Create(size, size, FORMAT_RGBA8888);
        uint8* _date = img->data;
        for (uint32 i1 = 0; i1 < size; ++i1)
        {
            uint8 blue = 0xFF * i1 / size;
            for (uint32 i2 = 0; i2 < size; ++i2)
            {
                *_date++ = 0xFF * i2 / size; // R channel, 0 to FF horizontally
                *_date++ = 0x00; // G channel
                *_date++ = blue; // B channel, 0 to FF vertically
                *_date++ = 0xFA; // A channel
            }
        }
        return img;
    }

    Image* Create888Image() const
    {
        uint32 size = 512;
        Image* img = Image::Create(size, size, FORMAT_RGB888);
        uint8* _date = img->data;
        for (uint32 i1 = 0; i1 < size; ++i1)
        {
            uint8 blue = 0xFF * i1 / size;
            for (uint32 i2 = 0; i2 < size; ++i2)
            {
                *_date++ = 0xFF * i2 / size; // R channel, 0 to FF horizontally
                *_date++ = 0x00; // G channel
                *_date++ = blue; // B channel, 0 to FF vertically
            }
        }
        return img;
    }

    Image* CreateA8Image() const
    {
        uint32 size = 512;
        Image* img = Image::Create(size, size, FORMAT_A8);
        uint8* _date = img->data;
        for (uint32 i1 = 0; i1 < size; ++i1)
        {
            uint8 p0 = (0xFF / 2) * i1 / size;
            for (uint32 i2 = 0; i2 < size; ++i2)
            {
                *_date++ = ((0xFF - p0) * i2 / size) + p0; // gradient from black to white horizontally, from black to gray vertically
            }
        }
        return img;
    }
};
