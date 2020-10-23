#include "Render/Image/LibHDRHelper.h"

#include "FileSystem/File.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageConvert.h"
#include "Utils/StringUtils.h"

namespace DAVA
{
namespace LibHDRDetails
{
const String kRadianceHeader = "#?RADIANCE";
const String kRGBEHeader = "#?RGBE";
const String kRadianceFormatEntry = "FORMAT";
const String kRadiance32Bit_RLE_RGBE = "32-BIT_RLE_RGBE";

struct RGBE
{
    uint8 components[4];
    uint8& operator[](uint32 i)
    {
        return components[i];
    }
    const uint8& operator[](uint32 i) const
    {
        return components[i];
    }
};

uint8* ReadScanline(uint8* ptr, uint32 width, RGBE* scanline);
Vector4 RGBEToFloat(const RGBE& data);
}

LibHDRHelper::LibHDRHelper()
    : ImageFormatInterface(ImageFormat::IMAGE_FORMAT_HDR, "Radiance (HDR)", { ".hdr" }, { FORMAT_RGBA32F })
{
}

bool LibHDRHelper::CanProcessFileInternal(File* infile) const
{
    String buffer(1024, 0);
    infile->ReadLine(&buffer[0], static_cast<uint32>(buffer.size()));
    return (strcmp(buffer.c_str(), LibHDRDetails::kRadianceHeader.c_str()) == 0) ||
    (strcmp(buffer.c_str(), LibHDRDetails::kRGBEHeader.c_str()) == 0);
}

ImageInfo LibHDRHelper::GetImageInfo(File* infile, Size2i& flip) const
{
    String buffer(1024, 0);
    infile->ReadLine(&buffer[0], static_cast<uint32>(buffer.size()));
    bool validHeader = (strcmp(buffer.c_str(), LibHDRDetails::kRadianceHeader.c_str()) == 0) ||
    (strcmp(buffer.c_str(), LibHDRDetails::kRGBEHeader.c_str()) == 0);

    if (!validHeader)
    {
        Logger::Error("HDR file contain invalid header: %s", buffer.c_str());
        return ImageInfo();
    }

    Size2i size(0, 0);
    bool dimensionsFound = false;
    bool validFormatFound = false;
    while (!infile->IsEof() && !(validFormatFound && dimensionsFound))
    {
        std::fill(buffer.begin(), buffer.end(), 0);
        infile->ReadLine(&buffer[0], static_cast<uint32>(buffer.size()));
        if (buffer.empty() || (buffer.front() == 0)) // skip empty lines
            continue;

        // Remove whitespace and transform to upper
        buffer.erase(std::remove_if(buffer.begin(), buffer.end(), [](int8 c) { return std::isspace(c) != 0; }), buffer.end());
        std::transform(buffer.begin(), buffer.end(), buffer.begin(), toupper);

        if (buffer.front() == '#') // skip comments
            continue;

        size_t eqPos = buffer.find('=');
        if (eqPos != String::npos) // parse property
        {
            String param = buffer.substr(0, eqPos);
            String value = buffer.substr(eqPos + 1, LibHDRDetails::kRadiance32Bit_RLE_RGBE.length());
            if (param == LibHDRDetails::kRadianceFormatEntry)
            {
                if (value == LibHDRDetails::kRadiance32Bit_RLE_RGBE)
                {
                    validFormatFound = true;
                }
                else
                {
                    Logger::Error("Invalid (or unsupported format) in HDR: %s = %s", param.c_str(), value.c_str());
                    return ImageInfo();
                }
            }
            continue;
        }

        size_t xpos = buffer.find('X');
        size_t ypos = buffer.find('Y');
        if ((xpos == String::npos) || (ypos == String::npos) || (xpos == 0) || (ypos == 0))
        {
            Logger::Error("Unsupported line found in HDR file: %s", buffer.c_str());
            return ImageInfo();
        }

        String xSize = buffer.substr(xpos + 1);
        String ySize = buffer.substr(ypos + 1);
        flip.dx = (buffer[xpos - 1] == '-') ? -1 : 1;
        flip.dy = (buffer[ypos - 1] == '-') ? -1 : 1;
        size.dx = ParseStringTo<int32>(xSize);
        size.dy = ParseStringTo<int32>(ySize);
        dimensionsFound = true;
    }

    ImageInfo result;
    result.format = PixelFormat::FORMAT_RGBA32F;
    result.faceCount = 1;
    result.mipmapsCount = 1;
    result.width = static_cast<uint32>(size.dx);
    result.height = static_cast<uint32>(size.dy);
    result.dataSize = 4 * sizeof(float32) * result.width * result.height;
    return result;
}

ImageInfo LibHDRHelper::GetImageInfo(File* infile) const
{
    Size2i flip(1, 1);
    return GetImageInfo(infile, flip);
}

eErrorCode LibHDRHelper::ReadFile(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const
{
    Size2i flip(1, 1);
    ImageInfo desc = GetImageInfo(infile, flip);

    if ((desc.width < 8) || (desc.width > 0x7fff))
        return eErrorCode::ERROR_READ_FAIL;

    Vector<LibHDRDetails::RGBE> rgbeData(desc.width * desc.height);
    {
        // inData contains RLE compressed data, provided size is just an estimation of the buffer
        // actual decoded data will be placed into `rgbeData` container
        Vector<uint8> inData(ImageUtils::GetSizeInBytes(desc.width, desc.height, PixelFormat::FORMAT_RGBA8888));
        infile->Read(inData.data(), static_cast<uint32>(inData.size()));

        uint8* ptr = inData.data();
        for (uint32 y = 0; y < desc.height; ++y)
        {
            uint32 row = (flip.dy == -1) ? y : (desc.height - 1 - y);
            LibHDRDetails::RGBE* rowPtr = rgbeData.data() + desc.width * row;
            ptr = LibHDRDetails::ReadScanline(ptr, desc.width, rowPtr);
        }
    }

    Image* image = Image::Create(desc.width, desc.height, PixelFormat::FORMAT_RGBA32F);
    image->mipmapLevel = 0;
    Vector4* floatData = reinterpret_cast<Vector4*>(image->data);
    for (const LibHDRDetails::RGBE& rgbe : rgbeData)
        *floatData++ = LibHDRDetails::RGBEToFloat(rgbe);

    imageSet.push_back(image);
    return eErrorCode::SUCCESS;
}

eErrorCode LibHDRHelper::WriteFileAsCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat pixelFormat, ImageQuality quality) const
{
    Logger::Error("[%s] Writing is not yet supported for HDR", __FUNCTION__);
    return eErrorCode::ERROR_WRITE_FAIL;
}

eErrorCode LibHDRHelper::WriteFile(const FilePath& fileName, const Vector<Image*>& imageSet, PixelFormat piexelFormat, ImageQuality quality) const
{
    Logger::Error("[%s] Writing is not yet supported for HDR", __FUNCTION__);
    return eErrorCode::ERROR_WRITE_FAIL;
}

/*
 * Internal implementation
 */
namespace LibHDRDetails
{
uint8* ReadScanline(uint8* ptr, uint32 width, RGBE* scanline)
{
    if (*ptr++ == 2)
    {
        (*scanline)[1] = *ptr++;
        (*scanline)[2] = *ptr++;

        ++ptr;

        for (uint32 i = 0; i < 4; ++i)
        {
            for (uint32 j = 0; j < width;)
            {
                uint8 code = *ptr++;
                if (code > 128)
                {
                    code &= 127;
                    uint8 val = *ptr++;
                    while (code--)
                        scanline[j++][i] = val;
                }
                else
                {
                    while (code--)
                        scanline[j++][i] = *ptr++;
                }
            }
        }
    }
    else
    {
        DVASSERT(0, "Legacy HDR files are not supported");
    }

    return ptr;
}

inline float32 ConvertComponent(int8 expo, uint8 val)
{
    return (expo > 0) ? static_cast<float32>(val * (1 << expo)) : static_cast<float32>(val) / static_cast<float32>(1 << -expo);
}

Vector4 RGBEToFloat(const RGBE& data)
{
    int8 expo = data[3] - 128;
    return Vector4(ConvertComponent(expo, data[0]), ConvertComponent(expo, data[1]), ConvertComponent(expo, data[2]), 256.0f) / 256.0f;
}
}
};
