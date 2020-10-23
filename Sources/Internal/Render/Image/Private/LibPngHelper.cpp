#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <iostream>

#include "Render/Image/LibPngHelper.h"

#include "Render/2D/Sprite.h"
#include "Render/Texture.h"
#include "FileSystem/FileSystem.h"
#include "Render/Image/ImageConvert.h"
#include "Render/PixelFormatDescriptor.h"

#include <libpng/png.h>

#if !defined(__DAVAENGINE_WINDOWS__)
#include <unistd.h>
#endif //#if !defined(__DAVAENGINE_WINDOWS__)

#define PNG_DEBUG 3

namespace
{
struct PngImageRawData
{
    DAVA::File* file;
};

void PngImageRead(png_structp pngPtr, png_bytep data, png_size_t size)
{
    PngImageRawData* self = static_cast<PngImageRawData*>(png_get_io_ptr(pngPtr));
    self->file->Read(data, static_cast<DAVA::uint32>(size));
}

void ReleaseWriteData(png_struct*& png_ptr, png_info*& info_ptr, unsigned char**& row_pointers, FILE*& fp, DAVA::Image*& convertedImage)
{
    free(row_pointers);
    row_pointers = nullptr;
    png_destroy_write_struct(&png_ptr, &info_ptr);
    png_ptr = nullptr;
    info_ptr = nullptr;
    if (fp != nullptr)
    {
        fclose(fp);
        fp = nullptr;
    }
    DAVA::SafeRelease(convertedImage);
}
}

void abort_(const char* s, ...)
{
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}

namespace DAVA
{
LibPngHelper::LibPngHelper()
    : ImageFormatInterface(ImageFormat::IMAGE_FORMAT_PNG, "PNG", { ".png" }, { FORMAT_RGBA8888, FORMAT_A8, FORMAT_A16 })
{
}

bool LibPngHelper::CanProcessFileInternal(File* infile) const
{
    unsigned char sig[8];
    infile->Read(sig, 8);
    bool retValue = (0 != png_check_sig(sig, 8));
    return retValue;
}

eErrorCode LibPngHelper::ReadFile(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const
{
    Image* image = new Image();
    eErrorCode innerRetCode = ReadPngFile(infile, image);

    if (innerRetCode == eErrorCode::SUCCESS)
    {
        image->mipmapLevel = loadingParams.firstMipmapIndex;
        imageSet.push_back(image);
    }
    else
    {
        SafeRelease(image);
    }

    return innerRetCode;
}

eErrorCode LibPngHelper::WriteFile(const FilePath& fileName, const Vector<Image*>& imageSet, PixelFormat format, ImageQuality quality) const
{
    // printf("* Writing PNG file (%d x %d): %s\n", width, height, file_name);
    DVASSERT(imageSet.size());
    int32 width = imageSet[0]->width;
    int32 height = imageSet[0]->height;
    uint8* imageData = imageSet[0]->data;
    Image* convertedImage = nullptr;
    if (FORMAT_RGB888 == imageSet[0]->format)
    {
        convertedImage = Image::Create(width, height, FORMAT_RGBA8888);
        ConvertDirect<RGB888, uint32, ConvertRGB888toRGBA8888> convert;
        convert(imageData, width, height, sizeof(RGB888) * width, convertedImage->data, width, height, sizeof(uint32) * width);
        imageData = convertedImage->data;
    }

    png_color_8 sig_bit;

    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;

    png_byte color_type = PNG_COLOR_TYPE_RGBA;
    png_byte bit_depth = 8;

    int32 bytes_for_color = 4;
    if (FORMAT_A8 == format)
    {
        color_type = PNG_COLOR_TYPE_GRAY;
        bytes_for_color = 1;
    }
    else if (FORMAT_A16 == format)
    {
        bit_depth = 16;
        color_type = PNG_COLOR_TYPE_GRAY;
        bytes_for_color = 2;
    }

    png_bytep* row_pointers = static_cast<png_bytep*>(malloc(sizeof(png_bytep) * height));

    for (int y = 0; y < height; y++)
    {
        // row_pointers[y] = (png_byte*) &data[y * width * 4];
        row_pointers[y] = reinterpret_cast<png_byte*>(&imageData[y * width * bytes_for_color]);
    }

    // create file
    FILE* fp = fopen(fileName.GetAbsolutePathname().c_str(), "wb");
    if (nullptr == fp)
    {
        Logger::Error("[LibPngHelper::WritePngFile] File %s could not be opened for writing", fileName.GetAbsolutePathname().c_str());
        ReleaseWriteData(png_ptr, info_ptr, row_pointers, fp, convertedImage);
        return eErrorCode::ERROR_FILE_NOTFOUND;
    }

    // initialize stuff
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

    if (nullptr == png_ptr)
    {
        Logger::Error("[LibPngHelper::WritePngFile] png_create_write_struct failed");
        ReleaseWriteData(png_ptr, info_ptr, row_pointers, fp, convertedImage);
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (nullptr == info_ptr)
    {
        Logger::Error("[LibPngHelper::WritePngFile] png_create_info_struct failed");
        ReleaseWriteData(png_ptr, info_ptr, row_pointers, fp, convertedImage);
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        Logger::Error("[LibPngHelper::WritePngFile] Error during init_io");
        ReleaseWriteData(png_ptr, info_ptr, row_pointers, fp, convertedImage);
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    png_init_io(png_ptr, fp);

    // write header
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        Logger::Error("[LibPngHelper::WritePngFile] Error during writing header");
        ReleaseWriteData(png_ptr, info_ptr, row_pointers, fp, convertedImage);
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    png_set_IHDR(png_ptr, info_ptr, width, height,
                 bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    if (FORMAT_A8 == format)
    {
        sig_bit.red = 0;
        sig_bit.green = 0;
        sig_bit.blue = 0;

        sig_bit.gray = 8;
        sig_bit.alpha = 0;
    }
    else if (FORMAT_A16 == format)
    {
        sig_bit.red = 0;
        sig_bit.green = 0;
        sig_bit.blue = 0;

        sig_bit.gray = 16;
        sig_bit.alpha = 0;
    }
    else
    {
        sig_bit.red = 8;
        sig_bit.green = 8;
        sig_bit.blue = 8;
        sig_bit.alpha = 8;
    }

    png_set_sBIT(png_ptr, info_ptr, &sig_bit);

    png_write_info(png_ptr, info_ptr);
    png_set_shift(png_ptr, &sig_bit);
    png_set_packing(png_ptr);

    // write bytes
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        Logger::Error("[LibPngHelper::WritePngFile] Error during writing bytes");
        ReleaseWriteData(png_ptr, info_ptr, row_pointers, fp, convertedImage);
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    png_write_image(png_ptr, row_pointers);

    // end write
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        Logger::Error("[LibPngHelper::WritePngFile] Error during end of write");
        ReleaseWriteData(png_ptr, info_ptr, row_pointers, fp, convertedImage);
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    png_write_end(png_ptr, nullptr);

    ReleaseWriteData(png_ptr, info_ptr, row_pointers, fp, convertedImage);
    return eErrorCode::SUCCESS;
}

eErrorCode LibPngHelper::WriteFileAsCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const
{
    Logger::Error("[LibPngHelper::WriteFileAsCubeMap] For png cubeMaps are not supported");
    return eErrorCode::ERROR_WRITE_FAIL;
}

ImageInfo LibPngHelper::GetImageInfo(File* infile) const
{
    if (!infile)
    {
        return ImageInfo();
    }

    uint8 sig[8];
    infile->Read(sig, 8);
    if (!png_check_sig(sig, 8))
    {
        return ImageInfo();
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (nullptr == png_ptr)
    {
        return ImageInfo();
    }

    png_set_option(png_ptr, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON); //disable "iCCP: known incorrect sRGB profile" warning

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (nullptr == info_ptr)
    {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return ImageInfo();
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return ImageInfo();
    }

    PngImageRawData raw;
    raw.file = infile;
    png_set_read_fn(png_ptr, &raw, PngImageRead);

    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    int bit_depth = 8;
    int color_type = PNG_COLOR_TYPE_RGBA;

    png_uint_32 width = 0;
    png_uint_32 height = 0;

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

    ImageInfo info;
    info.width = width;
    info.height = height;

    switch (color_type)
    {
    case PNG_COLOR_TYPE_RGB_ALPHA:
    {
        switch (bit_depth)
        {
        case 4:
            info.format = FORMAT_RGBA4444;
            break;
        case 8:
            info.format = FORMAT_RGBA8888;
            break;
        case 16:
            info.format = FORMAT_RGBA16161616;
            break;
        case 32:
            info.format = FORMAT_RGBA32323232;
            break;
        default:
            info.format = FORMAT_INVALID;
            break;
        }
        break;
    }
    case PNG_COLOR_TYPE_RGB:
    {
        switch (bit_depth)
        {
        case 5:
        case 6:
            info.format = FORMAT_RGB565;
            break;
        case 8:
            info.format = FORMAT_RGB888;
            break;
        default:
            info.format = FORMAT_INVALID;
            break;
        }
        break;
    }
    case PNG_COLOR_TYPE_GRAY:
    {
        info.format = FORMAT_A8;
        break;
    }
    case PNG_COLOR_TYPE_GRAY_ALPHA:
    {
        info.format = FORMAT_A16;
        break;
    }

    case PNG_COLOR_TYPE_PALETTE:
    {
        info.format = FORMAT_RGBA8888;
        break;
    }
    default:
    {
        info.format = FORMAT_INVALID;
        break;
    }
    }

    //==== temporary solution for legasy png loading =====

    switch (info.format)
    {
    case FORMAT_INVALID:
    case FORMAT_A8:
    case FORMAT_A16:
        //do nothing
        break;

    default:
        info.format = FORMAT_RGBA8888;
        break;
    }

    //==== temporary solution for legasy png loading =====

    // Clean up
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    info.dataSize = ImageUtils::GetSizeInBytes(width, height, info.format);
    info.mipmapsCount = 1;
    info.faceCount = 1;
    return info;
}

eErrorCode LibPngHelper::ReadPngFile(File* infile, Image* image, PixelFormat targetFormat /* = FORMAT_INVALID*/)
{
    DVASSERT(targetFormat == FORMAT_INVALID || targetFormat == FORMAT_RGBA8888);

    uint8 sig[8];
    infile->Read(sig, 8);

    if (!png_check_sig(sig, 8))
    {
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    png_structp png_ptr = nullptr;
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (nullptr == png_ptr)
    {
        return eErrorCode::ERROR_READ_FAIL; // out of memory
    }

    png_set_option(png_ptr, PNG_SKIP_sRGB_CHECK_PROFILE, PNG_OPTION_ON); //disable "iCCP: known incorrect sRGB profile" warning

    png_infop info_ptr = nullptr;
    info_ptr = png_create_info_struct(png_ptr);
    if (nullptr == info_ptr)
    {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return eErrorCode::ERROR_READ_FAIL; // out of memory
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    PngImageRawData raw;
    raw.file = infile;
    png_set_read_fn(png_ptr, &raw, PngImageRead);

    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                 &color_type, nullptr, nullptr, nullptr);

    image->width = width;
    image->height = height;

    // 1 bit images -> 8 bit
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);

    image->format = FORMAT_RGBA8888;
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        if (targetFormat == FORMAT_RGBA8888)
        {
            png_set_gray_to_rgb(png_ptr);
            png_set_filler(png_ptr, 0xFFFF, PNG_FILLER_AFTER);
        }
        else
        {
            image->format = (bit_depth == 16) ? FORMAT_A16 : FORMAT_A8;
            if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            {
                png_set_strip_alpha(png_ptr);
            }
        }
    }
    else if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(png_ptr);
        png_set_filler(png_ptr, 0xFFFF, PNG_FILLER_AFTER);
    }
    else if (color_type == PNG_COLOR_TYPE_RGB)
    {
        png_set_filler(png_ptr, 0xFFFF, PNG_FILLER_AFTER);
    }

    if (bit_depth > 8 && image->format != FORMAT_A16)
    {
        Logger::Error("Wrong image: must be 8bits on channel: %s", infile->GetFilename().GetAbsolutePathname().c_str());
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    {
        png_set_tRNS_to_alpha(png_ptr);
    }

    png_read_update_info(png_ptr, info_ptr);

    unsigned int rowbytes = static_cast<uint32>(png_get_rowbytes(png_ptr, info_ptr));
    image->dataSize = rowbytes * height;
    SafeDeleteArray(image->data);
    image->data = new uint8[image->dataSize];

    png_bytepp row_pointers = nullptr;
    row_pointers = static_cast<png_bytepp>(malloc(height * sizeof(png_bytep)));
    if (nullptr == row_pointers)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return eErrorCode::ERROR_READ_FAIL;
    }

    // set the individual row_pointers to point at the correct offsets

    for (int i = 0; i < static_cast<int>(height); ++i)
        row_pointers[i] = image->data + i * rowbytes;

    // now we can go ahead and just read the whole image
    png_read_image(png_ptr, row_pointers);

    // and we're done!  (png_read_end() can be omitted if no processing of
    // post-IDAT text/time/etc. is desired)

    // Clean up
    free(row_pointers);

    // Clean up
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    return eErrorCode::SUCCESS;
}
}