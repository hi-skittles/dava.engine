#include "Render/Image/LibTgaHelper.h"

#include "FileSystem/File.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageConvert.h"

namespace DAVA
{
static const uint8 MAX_BYTES_IN_PIXEL = 16;

LibTgaHelper::LibTgaHelper()
    : ImageFormatInterface(ImageFormat::IMAGE_FORMAT_TGA, "TGA",
                           { ".tga", ".tpic" },
                           { FORMAT_RGBA8888, FORMAT_RGBA5551, FORMAT_RGBA4444, FORMAT_RGB888,
                             FORMAT_RGB565, FORMAT_RGBA16161616, FORMAT_RGBA32323232, FORMAT_A8,
                             FORMAT_A16 })
{
}

DAVA::ImageInfo LibTgaHelper::GetImageInfo(File* infile) const
{
    DVASSERT(infile);

    TgaInfo tgaInfo;

    infile->Seek(0, File::SEEK_FROM_START);
    eErrorCode readResult = ReadTgaHeader(infile, tgaInfo);
    infile->Seek(0, File::SEEK_FROM_START);

    ImageInfo imageInfo;

    if (readResult == eErrorCode::SUCCESS)
    {
        imageInfo.height = tgaInfo.height;
        imageInfo.width = tgaInfo.width;
        imageInfo.dataSize = tgaInfo.width * tgaInfo.height * tgaInfo.bytesPerPixel;
        imageInfo.format = tgaInfo.pixelFormat;
        imageInfo.mipmapsCount = 1;
        imageInfo.faceCount = 1;
    }

    return imageInfo;
}

enum TgaHeaderSpec
{
    idlengthOffset = 0, // 1 byte, should be 0
    colorMapTypeOffset = 1, // 1 byte, should be 0
    imageTypeOffset = 2, // 1 byte, can be 2,3,10,11
    colorMapDataOffset = 3, // 5 bytes, should be 0,0,0,0,0
    originXOffset = 8, // 2 bytes, should be 0 for bottomleft etc., skip this
    originYOffset = 10, // 2 bytes, should be 0 for bottomleft etc., skip this
    widthOffset = 12, // 2 bytes, image width in pixels
    heightOffset = 14, // 2 bytes, image height in pixels
    bppOffset = 16, // 1 byte, bitsPerPixel, can be 8,16,24,32,64,128
    descriptorOffset = 17 // 1 byte, image origin corner and number of alpha bits
};

uint16 LowEndianToUint16(const uint8* p)
{
    return uint16(p[0] | (uint16(p[1]) << 8));
}

void Uint16ToLowEndian(const uint16 u16, uint8* p)
{
    p[0] = u16 & 0x00FF;
    p[1] = u16 >> 8;
}

DAVA::eErrorCode LibTgaHelper::ReadTgaHeader(const FilePath& filepath, TgaInfo& tgaInfo) const
{
    ScopedPtr<File> fileRead(File::Create(filepath, File::READ | File::OPEN));
    if (!fileRead)
    {
        return eErrorCode::ERROR_FILE_NOTFOUND;
    }

    fileRead->Seek(0, File::SEEK_FROM_START);
    return ReadTgaHeader(fileRead, tgaInfo);
}

DAVA::eErrorCode LibTgaHelper::ReadTgaHeader(File* infile, TgaInfo& tgaInfo) const
{
    Array<uint8, 18> fields;
    size_t bytesRead = infile->Read(&fields, static_cast<uint32>(fields.size()));
    if (bytesRead != fields.size())
        return eErrorCode::ERROR_READ_FAIL;

    static const Array<uint8, 5> zeroes = { { 0, 0, 0, 0, 0 } };
    if (Memcmp(&fields[idlengthOffset], &zeroes[0], 2) != 0 ||
        Memcmp(&fields[colorMapDataOffset], &zeroes[0], 5) != 0)
    {
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    // obtaining imageType
    tgaInfo.imageType = static_cast<TgaInfo::IMAGE_TYPE>(fields[imageTypeOffset]);
    if (tgaInfo.imageType != TgaInfo::GRAYSCALE &&
        tgaInfo.imageType != TgaInfo::TRUECOLOR &&
        tgaInfo.imageType != TgaInfo::COMPRESSED_GRAYSCALE &&
        tgaInfo.imageType != TgaInfo::COMPRESSED_TRUECOLOR)
    {
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    // obtaining width and height
    tgaInfo.width = LowEndianToUint16(&fields[widthOffset]); //tga uses low endianness
    tgaInfo.height = LowEndianToUint16(&fields[heightOffset]); //tga uses low endianness
    if (!tgaInfo.width || !tgaInfo.height)
    {
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    // obtaining bytes per pixel
    tgaInfo.bytesPerPixel = fields[bppOffset] >> 3;
    switch (tgaInfo.bytesPerPixel)
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 8:
    case 16:
        break;
    default:
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    // obtaining origin corner info
    uint8 descriptor = fields[descriptorOffset];
    tgaInfo.origin_corner = static_cast<TgaInfo::ORIGIN_CORNER>((descriptor >> 4) & 0x03);

    // obtaining number of alpha bits
    tgaInfo.alphaBits = descriptor & 0x0F;
    switch (tgaInfo.alphaBits)
    {
    case 0:
    case 1:
    case 4:
    case 8:
        break;
    default:
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    // defining pixel format
    tgaInfo.pixelFormat = DefinePixelFormat(tgaInfo);
    if (tgaInfo.pixelFormat == FORMAT_INVALID)
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;

    return eErrorCode::SUCCESS;
}

struct Convert_TgaARGB1555_to_RGBA5551
{
    inline void operator()(const uint16* input, uint16* output)
    {
        //arrr rrgg gggb bbbb --> rrrr rggg ggbb bbba

        //targa does not use alpha bit https://forums.adobe.com/thread/1303965?tstart=0
        *output = (*input << 1) | 0x1;
    }
};

struct Convert_RGBA5551_to_TgaARGB1555
{
    //rrrr rggg ggbb bbba --> arrr rrgg gggb bbbb
    inline void operator()(const uint16* input, uint16* output)
    {
        *output = (*input >> 1) | 0x8000;
    }
};

eErrorCode LibTgaHelper::ReadFile(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const
{
    DVASSERT(infile);

    TgaInfo tgaInfo;

    infile->Seek(0, File::SEEK_FROM_START);
    eErrorCode readResult = ReadTgaHeader(infile, tgaInfo);
    if (readResult != eErrorCode::SUCCESS)
        return readResult;

    Image* pImage = Image::Create(tgaInfo.width, tgaInfo.height, tgaInfo.pixelFormat);
    ScopedPtr<Image> image(pImage);

    if (tgaInfo.imageType == TgaInfo::COMPRESSED_TRUECOLOR || tgaInfo.imageType == TgaInfo::COMPRESSED_GRAYSCALE)
        readResult = ReadCompressedTga(infile, tgaInfo, image);
    else
        readResult = ReadUncompressedTga(infile, tgaInfo, image);

    if (readResult == eErrorCode::SUCCESS)
    {
        // color formats are stored by TGA with swapped red-blue channels,
        // excepting RGBA5551 that is stored as ARGB1555
        if (tgaInfo.imageType == TgaInfo::TRUECOLOR || tgaInfo.imageType == TgaInfo::COMPRESSED_TRUECOLOR)
        {
            if (tgaInfo.pixelFormat == FORMAT_RGBA5551)
            {
                uint32 pitch = image->width * tgaInfo.bytesPerPixel;
                ConvertDirect<uint16, uint16, Convert_TgaARGB1555_to_RGBA5551> swap;
                swap(image->data, image->width, image->height, pitch, image->data);
            }
            else
            {
                ImageConvert::SwapRedBlueChannels(image);
            }
        }

        SafeRetain(pImage);
        pImage->mipmapLevel = loadingParams.firstMipmapIndex;
        imageSet.push_back(pImage);
        return eErrorCode::SUCCESS;
    }

    return readResult;
}

PixelFormat LibTgaHelper::DefinePixelFormat(const TgaInfo& tgaInfo) const
{
    if (tgaInfo.imageType == TgaInfo::TRUECOLOR || tgaInfo.imageType == TgaInfo::COMPRESSED_TRUECOLOR)
    {
        switch (tgaInfo.bytesPerPixel)
        {
        case 2:
        {
            switch (tgaInfo.alphaBits)
            {
            case 0:
                return FORMAT_RGB565;
            case 1:
                return FORMAT_RGBA5551;
            case 4:
                return FORMAT_RGBA4444;
            default:
                return FORMAT_INVALID;
            }
        }
        case 3:
        {
            return (tgaInfo.alphaBits == 0) ? FORMAT_RGB888 : FORMAT_INVALID;
        }
        case 4:
        {
            return (tgaInfo.alphaBits == 8) ? FORMAT_RGBA8888 : FORMAT_INVALID;
        }
        case 8:
        {
            return FORMAT_RGBA16161616;
        }
        case 16:
        {
            return FORMAT_RGBA32323232;
        }
        default:
        {
            return FORMAT_INVALID;
        }
        }
    }
    else if (tgaInfo.imageType == TgaInfo::GRAYSCALE || tgaInfo.imageType == TgaInfo::COMPRESSED_GRAYSCALE)
    {
        switch (tgaInfo.bytesPerPixel)
        {
        case 1:
            return FORMAT_A8;
        case 2:
            return FORMAT_A16;
        default:
            return FORMAT_INVALID;
        }
    }
    else
        return FORMAT_INVALID;
}

eErrorCode LibTgaHelper::ReadUncompressedTga(File* infile, const TgaInfo& tgaInfo, ScopedPtr<Image>& image) const
{
    auto readSize = infile->Read(image->data, image->dataSize);
    if (readSize != image->dataSize)
    {
        return eErrorCode::ERROR_READ_FAIL;
    }

    switch (tgaInfo.origin_corner)
    {
    case TgaInfo::BOTTOM_LEFT:
        image->FlipVertical();
        break;

    case TgaInfo::BOTTOM_RIGHT:
        image->FlipVertical();
        image->FlipHorizontal();
        break;

    case TgaInfo::TOP_RIGHT:
        image->FlipHorizontal();
        break;

    case TgaInfo::TOP_LEFT:
        break;

    default:
        break;
    }

    return eErrorCode::SUCCESS;
}

eErrorCode LibTgaHelper::ReadCompressedTga(File* infile, const TgaInfo& tgaInfo, ScopedPtr<Image>& image) const
{
    uint8 chunkHeader;
    Array<uint8, MAX_BYTES_IN_PIXEL> pixelBuffer;

    ImageDataWriter dataWriter(image, tgaInfo);

    while (!dataWriter.AtEnd())
    {
        if (infile->Read(&chunkHeader, 1) != 1)
            return eErrorCode::ERROR_READ_FAIL;

        if (chunkHeader < 128) // is raw section
        {
            ++chunkHeader; // number of raw pixels

            for (uint8 i = 0; (i < chunkHeader) && !dataWriter.AtEnd(); ++i)
            {
                if (infile->Read(pixelBuffer.data(), tgaInfo.bytesPerPixel) != tgaInfo.bytesPerPixel)
                    return eErrorCode::ERROR_READ_FAIL;

                dataWriter.Write(pixelBuffer.data());
            }
        }
        else // is compressed section
        {
            chunkHeader -= 127; // number of repeated pixels

            if (infile->Read(pixelBuffer.data(), tgaInfo.bytesPerPixel) != tgaInfo.bytesPerPixel)
                return eErrorCode::ERROR_READ_FAIL;

            for (uint8 i = 0; (i < chunkHeader) && !dataWriter.AtEnd(); ++i)
            {
                dataWriter.Write(pixelBuffer.data());
            }
        }
    }
    return eErrorCode::SUCCESS;
}

eErrorCode LibTgaHelper::WriteFileAsCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat pixelFormat, ImageQuality quality) const
{
    Logger::Error("[%s] CubeMaps are not supported for TGA", __FUNCTION__);
    return DAVA::eErrorCode::ERROR_WRITE_FAIL;
}

eErrorCode LibTgaHelper::WriteFile(const FilePath& fileName, const Vector<Image*>& imageSet, PixelFormat piexelFormat, ImageQuality quality) const
{
    DVASSERT(imageSet.size() == 1);
    int32 width = imageSet[0]->width;
    int32 height = imageSet[0]->height;
    uint8* imageData = imageSet[0]->data;
    PixelFormat pixelFormat = imageSet[0]->format;
    DVASSERT(width && height && imageData);

    TgaInfo tgaInfo;
    tgaInfo.width = width;
    tgaInfo.height = height;
    tgaInfo.origin_corner = TgaInfo::TOP_LEFT; // saving as top-left avoids additional transformations from/to our Image format
    tgaInfo.pixelFormat = pixelFormat;
    switch (pixelFormat)
    {
    case FORMAT_RGBA8888:
        tgaInfo.bytesPerPixel = 4;
        tgaInfo.alphaBits = 8;
        tgaInfo.imageType = TgaInfo::TRUECOLOR;
        break;
    case FORMAT_RGBA5551:
        tgaInfo.bytesPerPixel = 2;
        tgaInfo.alphaBits = 1;
        tgaInfo.imageType = TgaInfo::TRUECOLOR;
        break;
    case FORMAT_RGBA4444:
        tgaInfo.bytesPerPixel = 2;
        tgaInfo.alphaBits = 4;
        tgaInfo.imageType = TgaInfo::TRUECOLOR;
        break;
    case FORMAT_RGB888:
        tgaInfo.bytesPerPixel = 3;
        tgaInfo.alphaBits = 0;
        tgaInfo.imageType = TgaInfo::TRUECOLOR;
        break;
    case FORMAT_RGB565:
        tgaInfo.bytesPerPixel = 2;
        tgaInfo.alphaBits = 0;
        tgaInfo.imageType = TgaInfo::TRUECOLOR;
        break;
    case FORMAT_RGBA16161616:
        tgaInfo.bytesPerPixel = 8;
        tgaInfo.alphaBits = 16;
        tgaInfo.imageType = TgaInfo::TRUECOLOR;
        break;
    case FORMAT_RGBA32323232:
        tgaInfo.bytesPerPixel = 16;
        tgaInfo.alphaBits = 32;
        tgaInfo.imageType = TgaInfo::TRUECOLOR;
        break;
    case FORMAT_A8:
        tgaInfo.bytesPerPixel = 1;
        tgaInfo.alphaBits = 8;
        tgaInfo.imageType = TgaInfo::GRAYSCALE;
        break;
    case FORMAT_A16:
        tgaInfo.bytesPerPixel = 2;
        tgaInfo.alphaBits = 8;
        tgaInfo.imageType = TgaInfo::GRAYSCALE;
        break;
    default:
        Logger::Error("[%s] %s is not supported", __FUNCTION__, GlobalEnumMap<PixelFormat>::Instance()->ToString(pixelFormat));
        DVASSERT(false);
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    std::unique_ptr<uint8[]> convertedData;
    if (tgaInfo.imageType == TgaInfo::TRUECOLOR)
    {
        convertedData.reset(new uint8[width * height * tgaInfo.bytesPerPixel]);
        uint32 pitch = width * tgaInfo.bytesPerPixel;
        if (pixelFormat == FORMAT_RGBA5551) // for RGBA5551, TGA stores it as ARGB1555
        {
            ConvertDirect<uint16, uint16, Convert_RGBA5551_to_TgaARGB1555> swap;
            swap(imageData, width, height, pitch, convertedData.get());
        }
        else
        {
            ImageConvert::SwapRedBlueChannels(pixelFormat, imageData, width, height, pitch, convertedData.get());
        }
        imageData = convertedData.get();
    }

    ScopedPtr<File> dstFile(File::Create(fileName, File::CREATE | File::WRITE));
    if (!dstFile)
    {
        Logger::Error("[%s] File %s could not be opened for writing", __FUNCTION__, fileName.GetAbsolutePathname().c_str());
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    auto res = WriteTgaHeader(dstFile, tgaInfo);
    if (res != eErrorCode::SUCCESS)
        return res;

    return WriteUncompressedTga(dstFile, tgaInfo, imageData);
}

DAVA::eErrorCode LibTgaHelper::WriteTgaHeader(File* dstFile, const TgaInfo& tgaInfo) const
{
    Array<uint8, 18> fields = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    fields[imageTypeOffset] = static_cast<uint8>(tgaInfo.imageType);
    Uint16ToLowEndian(tgaInfo.width, &fields[widthOffset]);
    Uint16ToLowEndian(tgaInfo.height, &fields[heightOffset]);
    fields[originYOffset] = fields[heightOffset]; // for TOP_LEFT, originX = 0, originY = height
    fields[originYOffset + 1] = fields[heightOffset + 1];
    fields[bppOffset] = tgaInfo.bytesPerPixel << 3;
    fields[descriptorOffset] = tgaInfo.alphaBits | (tgaInfo.origin_corner & 0x03) << 4;

    uint32 fieldsCount = static_cast<uint32>(fields.size());
    auto bytesWritten = dstFile->Write(&fields, fieldsCount);
    return (bytesWritten == fieldsCount) ? eErrorCode::SUCCESS : eErrorCode::ERROR_WRITE_FAIL;
}

eErrorCode LibTgaHelper::WriteUncompressedTga(File* dstFile, const TgaInfo& tgaInfo, const uint8* data) const
{
    auto dataSize = tgaInfo.width * tgaInfo.height * tgaInfo.bytesPerPixel;
    auto writeSize = dstFile->Write(data, dataSize);
    if (writeSize != dataSize)
    {
        return eErrorCode::ERROR_READ_FAIL;
    }

    return eErrorCode::SUCCESS;
}

LibTgaHelper::ImageDataWriter::ImageDataWriter(Image* img, const LibTgaHelper::TgaInfo& _tgaInfo)
    : tgaInfo(_tgaInfo)
{
    linesRemaining = tgaInfo.height;
    linePixelsRemaining = tgaInfo.width;
    isAtEnd = false;
    ResetPtr(img, tgaInfo);
}

void LibTgaHelper::ImageDataWriter::Write(uint8* pixel)
{
    if (!isAtEnd)
    {
        Memcpy(ptr, pixel, tgaInfo.bytesPerPixel);
        IncrementPtr();
    }
}

void LibTgaHelper::ImageDataWriter::ResetPtr(const Image* image, const LibTgaHelper::TgaInfo& tgaInfo)
{
    switch (tgaInfo.origin_corner)
    {
    case TgaInfo::BOTTOM_LEFT:
    {
        ptr = image->data + (tgaInfo.width * tgaInfo.bytesPerPixel) * (tgaInfo.height - 1);
        ptrInc = tgaInfo.bytesPerPixel;
        ptrNextLineJump = -(tgaInfo.width * tgaInfo.bytesPerPixel * 2);
        break;
    }
    case TgaInfo::BOTTOM_RIGHT:
    {
        ptr = image->data + (tgaInfo.width * tgaInfo.bytesPerPixel * tgaInfo.height) - tgaInfo.bytesPerPixel;
        ptrInc = -tgaInfo.bytesPerPixel;
        ptrNextLineJump = 0;
        break;
    }
    case TgaInfo::TOP_LEFT:
    {
        ptr = image->data;
        ptrInc = tgaInfo.bytesPerPixel;
        ptrNextLineJump = 0;
        break;
    }
    case TgaInfo::TOP_RIGHT:
    {
        ptr = image->data + ((tgaInfo.width - 1) * tgaInfo.bytesPerPixel);
        ptrInc = -tgaInfo.bytesPerPixel;
        ptrNextLineJump = tgaInfo.width * tgaInfo.bytesPerPixel * 2;
        break;
    }
    default:
    {
        DVASSERT(false && "Unknown ORIGIN_CORNER");
    }
    }
}

void LibTgaHelper::ImageDataWriter::IncrementPtr()
{
    ptr += ptrInc;
    if (--linePixelsRemaining == 0)
    {
        if (--linesRemaining > 0)
        {
            linePixelsRemaining = tgaInfo.width;
            ptr += ptrNextLineJump;
        }
        else
            isAtEnd = true;
    }
}
};
