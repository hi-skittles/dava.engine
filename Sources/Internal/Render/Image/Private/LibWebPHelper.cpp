#include "Render/Image/LibWebPHelper.h"
#include "Render/Image/Image.h"
#include "Render/PixelFormatDescriptor.h"


#include "FileSystem/File.h"

#include "webp/decode.h"
#include "webp/encode.h"

namespace DAVA
{
LibWebPHelper::LibWebPHelper()
    : ImageFormatInterface(ImageFormat::IMAGE_FORMAT_WEBP, "WEBP", { ".webp" }, { FORMAT_RGB888, FORMAT_RGBA8888 })
{
}

eErrorCode LibWebPHelper::ReadFile(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const
{
    DVASSERT(infile);

    WebPDecoderConfig config;
    WebPBitstreamFeatures* bitstream = &config.input;
    auto initCStatus = WebPInitDecoderConfig(&config);
    if (0 == initCStatus)
    {
        Logger::Error("[LibWebPHelper::ReadFile] Error in WebPInitDecpderConfig. File %s", infile->GetFilename().GetAbsolutePathname().c_str());
        return eErrorCode::ERROR_READ_FAIL;
    }

    infile->Seek(0, File::SEEK_FROM_START);
    uint32 dataSize = static_cast<uint32>(infile->GetSize());
    uint8_t* data = new uint8_t[dataSize];
    SCOPE_EXIT
    {
        SafeDeleteArray(data);
    };
    infile->Read(data, dataSize);
    infile->Seek(0, File::SEEK_FROM_START);

    WebPBitstreamFeatures local_features;
    if (nullptr == bitstream)
    {
        bitstream = &local_features;
    }

    auto bsStatus = WebPGetFeatures(data, dataSize, bitstream);
    if (bsStatus != VP8_STATUS_OK)
    {
        Logger::Error("[LibWebPHelper::ReadFile] File %s has wrong WebP header", infile->GetFilename().GetAbsolutePathname().c_str());
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    uint8_t* newData = nullptr;
    if (bitstream->has_alpha)
    {
        newData = WebPDecodeRGBA(data, dataSize, &bitstream->width, &bitstream->height);
    }
    else
    {
        newData = WebPDecodeRGB(data, dataSize, &bitstream->width, &bitstream->height);
    }
    if (nullptr == newData)
    {
        Logger::Error("[LibWebPHelper::ReadFile] Error during decompression of file %s into WebP.", infile->GetFilename().GetAbsolutePathname().c_str());
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    Image* image = new Image();

    if (bitstream->has_alpha)
    {
        image->format = FORMAT_RGBA8888;
    }
    else
    {
        image->format = FORMAT_RGB888;
    }
    image->width = bitstream->width;
    image->height = bitstream->height;
    image->data = newData;
    image->customDeleter = ::free;
    image->dataSize = ImageUtils::GetSizeInBytes(bitstream->width, bitstream->height, image->format);
    image->mipmapLevel = loadingParams.firstMipmapIndex;

    imageSet.push_back(image);

    return eErrorCode::SUCCESS;
}

eErrorCode LibWebPHelper::WriteFile(const FilePath& fileName, const Vector<Image*>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const
{
    DVASSERT(imageSet.size());
    const Image* original = imageSet[0];
    int32 width = original->width;
    int32 height = original->height;
    uint8_t* imageData = original->data;
    PixelFormat format = original->format;

    if (!(FORMAT_RGBA8888 == format || format == FORMAT_RGB888))
    {
        Logger::Error("[LibWebPHelper::WriteFile] Not supported format");
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    uint8_t* outData = nullptr;
    SCOPE_EXIT
    {
        SafeDeleteArray(outData);
    };
    uint32 outSize;
    int stride = ImageUtils::GetPitchInBytes(width, format);
    if (FORMAT_RGB888 == format)
    {
        if (quality == LOSSLESS_IMAGE_QUALITY)
        {
            outSize = static_cast<uint32>(WebPEncodeLosslessRGB(imageData, width, height, stride, &outData));
        }
        else
        {
            outSize = static_cast<uint32>(WebPEncodeRGB(imageData, width, height, stride, quality, &outData));
        }
    }
    else
    {
        if (quality == LOSSLESS_IMAGE_QUALITY)
        {
            outSize = static_cast<uint32>(WebPEncodeLosslessRGBA(imageData, width, height, stride, &outData));
        }
        else
        {
            outSize = static_cast<uint32>(WebPEncodeRGBA(imageData, width, height, stride, quality, &outData));
        }
    }

    if (nullptr == outData)
    {
        Logger::Error("[LibWebPHelper::WriteFile] Error during compression of WebP into file %s.", fileName.GetAbsolutePathname().c_str());
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    File* outFile = File::Create(fileName, File::CREATE | File::WRITE);
    if (nullptr == outFile)
    {
        Logger::Error("[LibWebPHelper::WriteFile] File %s could not be opened for writing", fileName.GetAbsolutePathname().c_str());
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    outFile->Write(outData, outSize);
    outFile->Release();

    return eErrorCode::SUCCESS;
}

eErrorCode LibWebPHelper::WriteFileAsCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const
{
    Logger::Error("[LibWebPHelper::WriteFileAsCubeMap] For WebP cubeMaps are not supported");
    return eErrorCode::ERROR_WRITE_FAIL;
}

DAVA::ImageInfo LibWebPHelper::GetImageInfo(File* infile) const
{
    WebPDecoderConfig config;
    WebPInitDecoderConfig(&config);
    WebPBitstreamFeatures* const bitstream = &config.input;

    DVASSERT(infile);

    infile->Seek(0, File::SEEK_FROM_START);
    uint32 dataSize = static_cast<uint32>(infile->GetSize());
    uint8_t* data = new uint8_t[dataSize];
    SCOPE_EXIT
    {
        SafeDeleteArray(data);
    };
    infile->Read(data, dataSize);
    infile->Seek(0, File::SEEK_FROM_START);

    auto bsStatus = WebPGetFeatures(data, dataSize, bitstream);
    if (bsStatus != VP8_STATUS_OK)
    {
        return ImageInfo();
    }

    ImageInfo info;
    info.height = bitstream->height;
    info.width = bitstream->width;
    if (bitstream->has_alpha)
    {
        info.format = FORMAT_RGBA8888;
    }
    else
    {
        info.format = FORMAT_RGB888;
    }
    info.dataSize = ImageUtils::GetSizeInBytes(bitstream->width, bitstream->height, info.format);
    info.mipmapsCount = 1;
    info.faceCount = 1;

    SafeDeleteArray(data);

    return info;
}
};
