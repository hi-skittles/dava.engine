#include "Render/Image/ImageConvert.h"
#include "Render/Image/ImageConverter.h"
#include "Render/Image/Image.h"
#include "Engine/Engine.h"
#include "Functional/Function.h"
#include "Math/HalfFloat.h"

namespace DAVA
{
uint32 ChannelFloatToInt(float32 ch)
{
    float32 clamped = Clamp(ch, 0.0f, 1.0f);
    return static_cast<uint32>(clamped * std::numeric_limits<uint8>::max());
}

float32 ChannelIntToFloat(uint32 ch)
{
    return (static_cast<float32>(ch) / std::numeric_limits<uint8>::max());
}

namespace ImageConvert
{
bool Normalize(PixelFormat format, const void* inData, uint32 width, uint32 height, uint32 pitch, void* outData)
{
    bool processed = true;
    switch (format)
    {
    case FORMAT_RGBA8888:
    {
        ConvertDirect<uint32, uint32, NormalizeRGBA8888> convert;
        convert(inData, width, height, pitch, outData, width, height, pitch);
        break;
    }
    case FORMAT_RGB16F:
    {
        ConvertDirect<RGB16F, RGB16F, NormalizeRGB16F> convert;
        convert(inData, width, height, pitch, outData, width, height, pitch);
        break;
    }
    case FORMAT_RGB32F:
    {
        ConvertDirect<RGB32F, RGB32F, NormalizeRGB32F> convert;
        convert(inData, width, height, pitch, outData, width, height, pitch);
        break;
    }
    case FORMAT_RGBA16F:
    {
        ConvertDirect<RGBA16F, RGBA16F, NormalizeRGBA16F> convert;
        convert(inData, width, height, pitch, outData, width, height, pitch);
        break;
    }
    case FORMAT_RGBA32F:
    {
        ConvertDirect<RGBA32F, RGBA32F, NormalizeRGBA32F> convert;
        convert(inData, width, height, pitch, outData, width, height, pitch);
        break;
    }
    default:
        Logger::Error("Normalize function not implemented for %s", PixelFormatDescriptor::GetPixelFormatString(format));
        processed = false;
    }
    return processed;
}

bool ConvertImage(const Image* srcImage, Image* dstImage)
{
    DVASSERT(srcImage);
    DVASSERT(dstImage);
    DVASSERT(srcImage->format != dstImage->format);
    DVASSERT(srcImage->width == dstImage->width);
    DVASSERT(srcImage->height == dstImage->height);

    PixelFormat srcFormat = srcImage->format;
    PixelFormat dstFormat = dstImage->format;

    ImageConverter* imageConverter = GetEngineContext()->imageConverter;
    if (imageConverter != nullptr)
    {
        if (imageConverter->CanConvert(srcFormat, dstFormat))
        {
            return imageConverter->Convert(srcImage, dstImage);
        }
        else if (imageConverter->CanConvert(srcFormat, PixelFormat::FORMAT_RGBA8888) && imageConverter->CanConvert(PixelFormat::FORMAT_RGBA8888, dstFormat))
        {
            ScopedPtr<Image> intermediateImage(Image::Create(srcImage->width, srcImage->height, FORMAT_RGBA8888));
            return imageConverter->Convert(srcImage, intermediateImage) && imageConverter->Convert(intermediateImage, dstImage);
        }
        else if (imageConverter->CanConvert(srcFormat, PixelFormat::FORMAT_RGBA8888) && (dstFormat != FORMAT_RGBA8888))
        {
            ScopedPtr<Image> intermediateImage(Image::Create(srcImage->width, srcImage->height, FORMAT_RGBA8888));
            return imageConverter->Convert(srcImage, intermediateImage) && ConvertImageDirect(intermediateImage, dstImage);
        }
        else if (imageConverter->CanConvert(PixelFormat::FORMAT_RGBA8888, dstFormat) && (srcFormat != FORMAT_RGBA8888))
        {
            ScopedPtr<Image> intermediateImage(Image::Create(srcImage->width, srcImage->height, FORMAT_RGBA8888));
            return ConvertImageDirect(srcImage, intermediateImage) && imageConverter->Convert(intermediateImage, dstImage);
        }
    }

    return ConvertImageDirect(srcImage, dstImage);
}

bool ConvertImageDirect(const Image* srcImage, Image* dstImage)
{
    return ConvertImageDirect(srcImage->format, dstImage->format,
                              srcImage->data, srcImage->width, srcImage->height,
                              ImageUtils::GetPitchInBytes(srcImage->width, srcImage->format),
                              dstImage->data, dstImage->width, dstImage->height,
                              ImageUtils::GetPitchInBytes(dstImage->width, dstImage->format));
}

bool ConvertImageDirect(PixelFormat inFormat, PixelFormat outFormat,
                        const void* inData, uint32 inWidth, uint32 inHeight, uint32 inPitch,
                        void* outData, uint32 outWidth, uint32 outHeight, uint32 outPitch)
{
    if (inFormat == FORMAT_RGBA5551 && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<uint16, uint32, ConvertRGBA5551toRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_RGBA4444 && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<uint16, uint32, ConvertRGBA4444toRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_RGB888 && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<RGB888, uint32, ConvertRGB888toRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_RGB565 && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<uint16, uint32, ConvertRGB565toRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_A8 && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<uint8, uint32, ConvertA8toRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_A16 && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<uint16, uint32, ConvertA16toRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_BGR888 && outFormat == FORMAT_RGB888)
    {
        ConvertDirect<BGR888, RGB888, ConvertBGR888toRGB888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_BGR888 && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<BGR888, uint32, ConvertBGR888toRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_BGRA8888 && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<BGRA8888, RGBA8888, ConvertBGRA8888toRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_RGBA8888 && outFormat == FORMAT_RGB888)
    {
        ConvertDirect<uint32, RGB888, ConvertRGBA8888toRGB888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_RGBA16161616 && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<RGBA16161616, uint32, ConvertRGBA16161616toRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_RGBA32323232 && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<RGBA32323232, uint32, ConvertRGBA32323232toRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_RGBA16F && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<RGBA16F, uint32, ConvertRGBA16FtoRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_RGBA32F && outFormat == FORMAT_RGBA8888)
    {
        ConvertDirect<RGBA32F, uint32, ConvertRGBA32FtoRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_RGBA8888 && outFormat == FORMAT_RGBA16F)
    {
        ConvertDirect<uint32, RGBA16F, ConvertRGBA8888toRGBA16F> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else if (inFormat == FORMAT_RGBA8888 && outFormat == FORMAT_RGBA32F)
    {
        ConvertDirect<uint32, RGBA32F, ConvertRGBA8888toRGBA32F> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        return true;
    }
    else
    {
        Logger::FrameworkDebug("Unsupported image conversion from format %d to %d", inFormat, outFormat);
        return false;
    }
}

bool CanConvertDirect(PixelFormat inFormat, PixelFormat outFormat)
{
    return ConvertImageDirect(inFormat, outFormat, nullptr, 0, 0, 0, nullptr, 0, 0, 0);
}

bool CanConvertFromTo(PixelFormat inFormat, PixelFormat outFormat)
{
    ImageConverter* imageConverter = GetEngineContext()->imageConverter;
    if (imageConverter != nullptr)
    {
        if (imageConverter->CanConvert(inFormat, outFormat))
        {
            return true;
        }
        else if (imageConverter->CanConvert(inFormat, PixelFormat::FORMAT_RGBA8888) && imageConverter->CanConvert(PixelFormat::FORMAT_RGBA8888, outFormat))
        {
            return true;
        }
        else if (imageConverter->CanConvert(inFormat, PixelFormat::FORMAT_RGBA8888) && (outFormat != FORMAT_RGBA8888))
        {
            return CanConvertDirect(FORMAT_RGBA8888, outFormat);
        }
        else if (imageConverter->CanConvert(PixelFormat::FORMAT_RGBA8888, outFormat) && (inFormat != FORMAT_RGBA8888))
        {
            return CanConvertDirect(inFormat, FORMAT_RGBA8888);
        }
    }

    return CanConvertDirect(inFormat, outFormat);
}

void SwapRedBlueChannels(const Image* srcImage, const Image* dstImage /* = nullptr*/)
{
    DVASSERT(srcImage);

    if (dstImage)
    {
        DVASSERT(PixelFormatDescriptor::GetPixelFormatSizeInBits(dstImage->format) == PixelFormatDescriptor::GetPixelFormatSizeInBits(srcImage->format));
        DVASSERT(srcImage->width == dstImage->width);
        DVASSERT(srcImage->height == dstImage->height);
    }

    SwapRedBlueChannels(srcImage->format, srcImage->data, srcImage->width, srcImage->height,
                        ImageUtils::GetPitchInBytes(srcImage->width, srcImage->format),
                        dstImage ? dstImage->data : nullptr);
}

void SwapRedBlueChannels(PixelFormat format, void* srcData, uint32 width, uint32 height, uint32 pitch, void* dstData /* = nullptr*/)
{
    if (!dstData)
        dstData = srcData;

    switch (format)
    {
    case FORMAT_RGB888:
    {
        ConvertDirect<BGR888, RGB888, ConvertBGR888toRGB888> swap;
        swap(srcData, width, height, pitch, dstData);
        return;
    }
    case FORMAT_RGBA8888:
    {
        ConvertDirect<BGRA8888, RGBA8888, ConvertBGRA8888toRGBA8888> swap;
        swap(srcData, width, height, pitch, dstData);
        return;
    }
    case FORMAT_RGBA4444:
    {
        ConvertDirect<uint16, uint16, ConvertBGRA4444toRGBA4444> swap;
        swap(srcData, width, height, pitch, dstData);
        return;
    }
    case FORMAT_RGB565:
    {
        ConvertDirect<uint16, uint16, ConvertBGR565toRGB565> swap;
        swap(srcData, width, height, pitch, dstData);
        return;
    }
    case FORMAT_RGBA16161616:
    {
        ConvertDirect<RGBA16161616, RGBA16161616, ConvertBGRA16161616toRGBA16161616> swap;
        swap(srcData, width, height, pitch, dstData);
        return;
    }
    case FORMAT_RGBA32323232:
    {
        ConvertDirect<RGBA32323232, RGBA32323232, ConvertBGRA32323232toRGBA32323232> swap;
        swap(srcData, width, height, pitch, dstData);
        return;
    }
    case FORMAT_A8:
    case FORMAT_A16:
    {
        // do nothing for grayscale images
        return;
    }
    default:
    {
        Logger::Error("Image color exchanging is not supported for format %d", format);
        return;
    }
    }
}

bool DownscaleTwiceBillinear(PixelFormat inFormat, PixelFormat outFormat,
                             const void* inData, uint32 inWidth, uint32 inHeight, uint32 inPitch,
                             void* outData, uint32 outWidth, uint32 outHeight, uint32 outPitch, bool normalize)
{
    if ((inFormat == FORMAT_RGBA8888) && (outFormat == FORMAT_RGBA8888))
    {
        if (normalize)
        {
            ConvertDownscaleTwiceBillinear<uint32, uint32, uint32, UnpackRGBA8888, PackNormalizedRGBA8888> convert;
            convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        }
        else
        {
            ConvertDownscaleTwiceBillinear<uint32, uint32, uint32, UnpackRGBA8888, PackRGBA8888> convert;
            convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
        }
    }
    else if ((inFormat == FORMAT_RGBA8888) && (outFormat == FORMAT_RGBA4444))
    {
        ConvertDownscaleTwiceBillinear<uint32, uint16, uint32, UnpackRGBA8888, PackRGBA4444> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
    }
    else if ((inFormat == FORMAT_RGBA4444) && (outFormat == FORMAT_RGBA8888))
    {
        ConvertDownscaleTwiceBillinear<uint16, uint32, uint32, UnpackRGBA4444, PackRGBA8888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
    }
    else if ((inFormat == FORMAT_A8) && (outFormat == FORMAT_A8))
    {
        ConvertDownscaleTwiceBillinear<uint8, uint8, uint32, UnpackA8, PackA8> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
    }
    else if ((inFormat == FORMAT_RGB888) && (outFormat == FORMAT_RGB888))
    {
        ConvertDownscaleTwiceBillinear<RGB888, RGB888, uint32, UnpackRGB888, PackRGB888> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
    }
    else if ((inFormat == FORMAT_RGBA5551) && (outFormat == FORMAT_RGBA5551))
    {
        ConvertDownscaleTwiceBillinear<uint16, uint16, uint32, UnpackRGBA5551, PackRGBA5551> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
    }
    else if ((inFormat == FORMAT_RGBA16161616) && (outFormat == FORMAT_RGBA16161616))
    {
        ConvertDownscaleTwiceBillinear<RGBA16161616, RGBA16161616, uint32, UnpackRGBA16161616, PackRGBA16161616> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
    }
    else if ((inFormat == FORMAT_RGBA32323232) && (outFormat == FORMAT_RGBA32323232))
    {
        ConvertDownscaleTwiceBillinear<RGBA32323232, RGBA32323232, uint64, UnpackRGBA32323232, PackRGBA32323232> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
    }
    else if ((inFormat == FORMAT_RGBA16F) && (outFormat == FORMAT_RGBA16F))
    {
        ConvertDownscaleTwiceBillinear<RGBA16F, RGBA16F, float32, UnpackRGBA16F, PackRGBA16F> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
    }
    else if ((inFormat == FORMAT_RGBA32F) && (outFormat == FORMAT_RGBA32F))
    {
        ConvertDownscaleTwiceBillinear<RGBA32F, RGBA32F, float32, UnpackRGBA32F, PackRGBA32F> convert;
        convert(inData, inWidth, inHeight, inPitch, outData, outWidth, outHeight, outPitch);
    }
    else
    {
        Logger::Error("Downscale from %s to %s is not implemented", PixelFormatDescriptor::GetPixelFormatString(inFormat), PixelFormatDescriptor::GetPixelFormatString(outFormat));
        return false;
    }

    return true;
}

Image* DownscaleTwiceBillinear(const Image* source, bool isNormalMap /*= false*/)
{
    DVASSERT(source != nullptr);

    PixelFormat pixelFormat = source->GetPixelFormat();
    uint32 sWidth = source->GetWidth();
    uint32 sHeigth = source->GetHeight();
    uint32 dWidth = sWidth >> 1;
    uint32 dHeigth = sHeigth >> 1;

    Image* destination = Image::Create(dWidth, dHeigth, pixelFormat);
    if (destination != nullptr)
    {
        uint32 pitchMultiplier = PixelFormatDescriptor::GetPixelFormatSizeInBits(pixelFormat);
        bool downscaled = DownscaleTwiceBillinear(pixelFormat, pixelFormat, source->GetData(), sWidth, sHeigth, sWidth * pitchMultiplier / 8, destination->GetData(), dWidth, dHeigth, dWidth * pitchMultiplier / 8, isNormalMap);
        if (downscaled == false)
        {
            SafeRelease(destination);
        }
    }

    return destination;
}

void ResizeRGBA8Billinear(const uint32* inPixels, uint32 w, uint32 h, uint32* outPixels, uint32 w2, uint32 h2)
{
    int32 a, b, c, d, x, y, index;
    float32 x_ratio = (static_cast<float32>(w - 1)) / w2;
    float32 y_ratio = (static_cast<float32>(h - 1)) / h2;
    float32 x_diff, y_diff, blue, red, green, alpha;
    uint32 offset = 0;
    for (uint32 i = 0; i < h2; i++)
    {
        for (uint32 j = 0; j < w2; j++)
        {
            x = static_cast<int32>(x_ratio * j);
            y = static_cast<int32>(y_ratio * i);
            x_diff = (x_ratio * j) - x;
            y_diff = (y_ratio * i) - y;
            index = (y * w + x);
            a = inPixels[index];
            b = inPixels[index + 1];
            c = inPixels[index + w];
            d = inPixels[index + w + 1];

            blue = (a & 0xff) * (1 - x_diff) * (1 - y_diff) + (b & 0xff) * (x_diff) * (1 - y_diff) +
            (c & 0xff) * (y_diff) * (1 - x_diff) + (d & 0xff) * (x_diff * y_diff);

            green = ((a >> 8) & 0xff) * (1 - x_diff) * (1 - y_diff) + ((b >> 8) & 0xff) * (x_diff) * (1 - y_diff) +
            ((c >> 8) & 0xff) * (y_diff) * (1 - x_diff) + ((d >> 8) & 0xff) * (x_diff * y_diff);

            red = ((a >> 16) & 0xff) * (1 - x_diff) * (1 - y_diff) + ((b >> 16) & 0xff) * (x_diff) * (1 - y_diff) +
            ((c >> 16) & 0xff) * (y_diff) * (1 - x_diff) + ((d >> 16) & 0xff) * (x_diff * y_diff);

            alpha = ((a >> 24) & 0xff) * (1 - x_diff) * (1 - y_diff) + ((b >> 24) & 0xff) * (x_diff) * (1 - y_diff) +
            ((c >> 24) & 0xff) * (y_diff) * (1 - x_diff) + ((d >> 24) & 0xff) * (x_diff * y_diff);

            outPixels[offset++] =
            (((static_cast<uint32>(alpha)) << 24) & 0xff000000) |
            (((static_cast<uint32>(red)) << 16) & 0xff0000) |
            (((static_cast<uint32>(green)) << 8) & 0xff00) |
            (static_cast<uint32>(blue));
        }
    }
}

inline float32 ReadFloatDirect(uint8* ptr)
{
    return *(reinterpret_cast<float*>(ptr));
}

inline float32 ReadHalfFloat(uint8* ptr)
{
    return Float16Compressor::Decompress(*(reinterpret_cast<uint16_t*>(ptr)));
}

inline void WriteFloatDirect(uint8* ptr, float32 value)
{
    *(reinterpret_cast<float*>(ptr)) = value;
}

inline void WriteHalfFloat(uint8* ptr, float32 value)
{
    *(reinterpret_cast<uint16_t*>(ptr)) = Float16Compressor::Compress(value);
}

inline void ConvertFloatPixel(uint32 inChannels, uint32 inSize, uint32 outChannels, uint32 outSize, uint8* inData, uint8* outData)
{
    auto readFunction = (inSize == 4) ? &ReadFloatDirect : &ReadHalfFloat;
    auto writeFunction = (outSize == 4) ? &WriteFloatDirect : &WriteHalfFloat;

    float32 in[4] = {};
    for (uint32 c = 0; c < inChannels; ++c)
        in[c] = readFunction(inData + c * inSize);

    for (uint32 c = 0; c < outChannels; ++c)
        writeFunction(outData + c * outSize, in[c]);
}

bool ConvertFloatFormats(uint32 width, uint32 height, PixelFormat inFormat, PixelFormat outFormat, void* inData, void* outData)
{
    if (!PixelFormatDescriptor::IsFloatPixelFormat(inFormat) || !PixelFormatDescriptor::IsFloatPixelFormat(outFormat))
    {
        Logger::Error("Formats being converted are not compatible");
        return false;
    }

    uint32 inChannels = 0;
    uint32 inSize = 0;
    uint32 inPitch = 0;
    PixelFormatDescriptor::GetFloatFormatInfo(width, inFormat, inChannels, inSize, inPitch);
    DVASSERT(inChannels <= 4);

    if ((inSize != 2) && (inSize != 4))
    {
        Logger::Error("Invalid channel size for the input float texture format");
        return false;
    }

    uint32 outChannels = 0;
    uint32 outSize = 0;
    uint32 outPitch = 0;
    PixelFormatDescriptor::GetFloatFormatInfo(width, outFormat, outChannels, outSize, outPitch);
    DVASSERT(outChannels <= 4);

    if ((outSize != 2) && (outSize != 4))
    {
        Logger::Error("Invalid channel size for the otuput float texture format");
        return false;
    }

    uint8* inPtr = reinterpret_cast<uint8*>(inData);
    uint8* outPtr = reinterpret_cast<uint8*>(outData);
    for (uint32 y = 0; y < height; ++y)
    {
        uint8* inRowPtr = inPtr + y * inPitch;
        uint8* outRowPtr = outPtr + y * outPitch;
        for (uint32 x = 0; x < width; ++x)
        {
            ConvertFloatPixel(inChannels, inSize, outChannels, outSize, inRowPtr, outRowPtr);
            inRowPtr += inSize * inChannels;
            outRowPtr += outSize * outChannels;
        }
    }

    return true;
}

} // namespace ImageConvert
} // namespace DAVA
