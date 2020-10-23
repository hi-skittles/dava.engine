#include "Render/Image/Image.h"
#include "Render/Image/LibJpegHelper.h"
#include "Render/Image/ImageConvert.h"

#include "Render/Texture.h"

#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"

#include <stdlib.h>
#include <stdio.h>

#include "libjpeg/jpeglib.h"
#include "libjpeg/jerror.h"
#include <setjmp.h>

namespace DAVA
{
struct jpegErrorManager
{
    // "public" fields
    struct jpeg_error_mgr pub;

    // for return to caller
    jmp_buf setjmp_buffer;
};

char jpegLastErrorMsg[JMSG_LENGTH_MAX];

void jpegErrorExit(j_common_ptr cinfo)
{
    // cinfo->err actually points to a jpegErrorManager struct
    jpegErrorManager* myerr = reinterpret_cast<jpegErrorManager*>(cinfo->err);
    // note : *(cinfo->err) is now equivalent to myerr->pub

    // output_message is a method to print an error message
    //(* (cinfo->err->output_message) ) (cinfo);

    // Create the message
    (*(cinfo->err->format_message))(cinfo, jpegLastErrorMsg);

    // Jump to the setjmp point
    longjmp(myerr->setjmp_buffer, 1);
}

LibJpegHelper::LibJpegHelper()
    : ImageFormatInterface(ImageFormat::IMAGE_FORMAT_JPEG, "JPG", { ".jpg", ".jpeg" }, { FORMAT_RGB888, FORMAT_A8 })
{
}

eErrorCode LibJpegHelper::ReadFile(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const
{
#if defined(__DAVAENGINE_ANDROID__) || defined(__DAVAENGINE_IOS__)
    // Magic. Allow LibJpeg to use large memory buffer to prevent using temp file.
    setenv("JPEGMEM", "10M", TRUE);
    SCOPE_EXIT
    {
        unsetenv("JPEGMEM");
    };
#endif

    DVASSERT(infile);

    jpeg_decompress_struct cinfo;
    jpegErrorManager jerr;

    infile->Seek(0, File::SEEK_FROM_START);
    uint32 fileSize = static_cast<uint32>(infile->GetSize());
    uint8* fileBuffer = new uint8[fileSize];
    infile->Read(fileBuffer, fileSize);
    infile->Seek(0, File::SEEK_FROM_START);

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpegErrorExit;

    ScopedPtr<Image> image(new Image());
    image->mipmapLevel = loadingParams.firstMipmapIndex;

    //set error handling block, which will be called in case of fail of jpeg_start_decompress,jpeg_read_scanlines...
    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_decompress(&cinfo);
        SafeDeleteArray(fileBuffer);

        Logger::Error("[LibJpegHelper::ReadFile] File %s has wrong jpeg header", infile->GetFilename().GetAbsolutePathname().c_str());
        Logger::Error("[LibJpegHelper::ReadFile] Internal Error %d. Look it in jerror.h", cinfo.err->msg_code);

        if (J_MESSAGE_CODE::JERR_TFILE_CREATE == cinfo.err->msg_code)
        {
            Logger::Error("Unable to create temporary file. Seems you need more JPEGMEM");
        }

        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

    jpeg_create_decompress(&cinfo);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

    jpeg_mem_src(&cinfo, fileBuffer, fileSize);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    PixelFormat format = FORMAT_INVALID;
    if (cinfo.jpeg_color_space == JCS_GRAYSCALE)
    {
        switch (cinfo.output_components)
        {
        case 1:
            format = FORMAT_A8;
            break;
        case 2:
            format = FORMAT_A16;
            break;
        default:
            break;
        }
    }
    else
    {
        if (cinfo.output_components == 3)
            format = FORMAT_RGB888;
    }

    if (format == FORMAT_INVALID)
    {
        Logger::Error("[%s] Unable to detect format for %s", infile->GetFilename().GetAbsolutePathname().c_str());
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    image->width = cinfo.image_width;
    image->height = cinfo.image_height;
    image->format = format;
    DVASSERT(cinfo.num_components == PixelFormatDescriptor::GetPixelFormatSizeInBits(format) / 8);

    //as image->data will be rewrited, need to erase present buffer
    SafeDeleteArray(image->data);
    image->dataSize = cinfo.output_width * cinfo.output_height * cinfo.num_components;
    image->data = new uint8[image->dataSize];

    JSAMPROW output_data;
    unsigned int scanline_len = cinfo.output_width * cinfo.output_components;
    unsigned int scanline_count = 0;
    while (cinfo.output_scanline < cinfo.output_height)
    {
        output_data = (image->data + (scanline_count * scanline_len));
        jpeg_read_scanlines(&cinfo, &output_data, 1);
        scanline_count++;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    SafeDeleteArray(fileBuffer);
    imageSet.push_back(SafeRetain(image.get()));
    return eErrorCode::SUCCESS;
}

eErrorCode LibJpegHelper::WriteFileAsCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const
{
    Logger::Error("[LibJpegHelper::WriteFileAsCubeMap] For jpeg cubeMaps are not supported");
    return eErrorCode::ERROR_WRITE_FAIL;
}

eErrorCode LibJpegHelper::WriteFile(const FilePath& fileName, const Vector<Image*>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const
{
    DVASSERT(imageSet.size());
    const Image* original = imageSet[0];
    int32 width = original->width;
    int32 height = original->height;
    uint8* imageData = original->data;
    PixelFormat format = original->format;
    Image* convertedImage = NULL;
    if (!(FORMAT_RGB888 == original->format || FORMAT_A8 == original->format))
    {
        if (FORMAT_RGBA8888 == original->format)
        {
            convertedImage = Image::Create(width, height, FORMAT_RGB888);
            ConvertDirect<uint32, RGB888, ConvertRGBA8888toRGB888> convert;
            convert(imageData, width, height, sizeof(uint32) * width, convertedImage->data, width, height, sizeof(RGB888) * width);
        }
        else if (FORMAT_A16 == original->format)
        {
            convertedImage = Image::Create(width, height, FORMAT_A8);
            ConvertDirect<uint16, uint8, ConvertA16toA8> convert;
            convert(imageData, width, height, sizeof(uint16) * width, convertedImage->data, width, height, sizeof(uint8) * width);
        }
        DVASSERT(convertedImage);
        imageData = convertedImage->data;
        format = convertedImage->format;
    }

    jpeg_compress_struct cinfo;
    jpegErrorManager jerr;

    JSAMPROW row_pointer[1];
    FILE* outfile = fopen(fileName.GetAbsolutePathname().c_str(), "wb");

    if (nullptr == outfile)
    {
        Logger::Error("[LibJpegHelper::WriteJpegFile] File %s could not be opened for writing", fileName.GetAbsolutePathname().c_str());
        SafeRelease(convertedImage);
        return eErrorCode::ERROR_FILE_NOTFOUND;
    }
    cinfo.err = jpeg_std_error(&jerr.pub);

    jerr.pub.error_exit = jpegErrorExit;
    // Establish the setjmp return context for my_error_exit to use.
    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_compress(&cinfo);
        fclose(outfile);
        Logger::Error("[LibJpegHelper::WriteJpegFile] Error during compression of jpeg into file %s.", fileName.GetAbsolutePathname().c_str());
        SafeRelease(convertedImage);
        return eErrorCode::ERROR_WRITE_FAIL;
    }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

    jpeg_create_compress(&cinfo);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

    jpeg_stdio_dest(&cinfo, outfile);

    // Setting the parameters of the output file here
    cinfo.image_width = width;
    cinfo.image_height = height;

    cinfo.in_color_space = JCS_RGB;
    int colorComponents = 3;
    if (format == FORMAT_A8)
    {
        cinfo.in_color_space = JCS_GRAYSCALE;
        colorComponents = 1;
    }

    cinfo.input_components = colorComponents;

    jpeg_set_defaults(&cinfo);
    cinfo.num_components = colorComponents;
    //cinfo.data_precision = 4;
    cinfo.dct_method = JDCT_FLOAT;

    //The quality value ranges from 0..100. If "force_baseline" is TRUE, the computed quantization table entries are limited to 1..255 for JPEG baseline compatibility.
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);
    while (cinfo.next_scanline < cinfo.image_height)
    {
        row_pointer[0] = &imageData[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(outfile);
    SafeRelease(convertedImage);
    return eErrorCode::SUCCESS;
}

DAVA::ImageInfo LibJpegHelper::GetImageInfo(File* infile) const
{
    jpeg_decompress_struct cinfo;
    jpegErrorManager jerr;

    DVASSERT(infile);

    infile->Seek(0, File::SEEK_FROM_START);
    uint32 fileSize = static_cast<uint32>(infile->GetSize());
    uint8* fileBuffer = new uint8[fileSize];
    infile->Read(fileBuffer, fileSize);
    cinfo.err = jpeg_std_error(&jerr.pub);

    jerr.pub.error_exit = jpegErrorExit;
    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_decompress(&cinfo);
        SafeDeleteArray(fileBuffer);
        infile->Seek(0, File::SEEK_FROM_START);
        return ImageInfo();
    }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

    jpeg_create_decompress(&cinfo);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

    jpeg_mem_src(&cinfo, fileBuffer, fileSize);
    jpeg_read_header(&cinfo, TRUE);
    infile->Seek(0, File::SEEK_FROM_START);

    ImageInfo info;
    info.width = cinfo.image_width;
    info.height = cinfo.image_height;
    switch (cinfo.out_color_space)
    {
    case JCS_RGB:
        info.format = FORMAT_RGB888;
        break;
    case JCS_GRAYSCALE:
        info.format = FORMAT_A8;
        break;
    default:
        info.format = FORMAT_INVALID;
    }
    info.dataSize = static_cast<uint32>(cinfo.src->bytes_in_buffer);
    info.mipmapsCount = 1;
    info.faceCount = 1;

    jpeg_destroy_decompress(&cinfo);
    SafeDeleteArray(fileBuffer);

    return info;
}
};
