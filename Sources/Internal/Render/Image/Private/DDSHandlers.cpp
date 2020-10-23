#include "Render/Image/Private/DDSHandlers.h"

#include "Render/Image/Image.h"
#include "Render/Image/ImageConvert.h"
#include "Render/Texture.h"
#include "Render/PixelFormatDescriptor.h"

#include "Utils/CRC32.h"
#include "Utils/StringFormat.h"

#include "Logger/Logger.h"

namespace DAVA
{

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) (static_cast<uint32>(static_cast<uint8>(ch0)) | (static_cast<uint32>(static_cast<uint8>(ch1)) << 8) | (static_cast<uint32>(static_cast<uint8>(ch2)) << 16) | (static_cast<uint32>(static_cast<uint8>(ch3)) << 24))
#endif

const uint32 FOURCC_CRC = MAKEFOURCC('C', 'R', 'C', '_');

namespace dds
{
const uint32 FOURCC_DDS = MAKEFOURCC('D', 'D', 'S', ' ');

const uint32 FOURCC_DXT1 = MAKEFOURCC('D', 'X', 'T', '1');
const uint32 FOURCC_DXT2 = MAKEFOURCC('D', 'X', 'T', '2');
const uint32 FOURCC_DXT3 = MAKEFOURCC('D', 'X', 'T', '3');
const uint32 FOURCC_DXT4 = MAKEFOURCC('D', 'X', 'T', '4');
const uint32 FOURCC_DXT5 = MAKEFOURCC('D', 'X', 'T', '5');
const uint32 FOURCC_RXGB = MAKEFOURCC('R', 'X', 'G', 'B');
const uint32 FOURCC_ATI1 = MAKEFOURCC('A', 'T', 'I', '1');
const uint32 FOURCC_ATI2 = MAKEFOURCC('A', 'T', 'I', '2');
const uint32 FOURCC_BC4U = MAKEFOURCC('B', 'C', '4', 'U');
const uint32 FOURCC_BC4S = MAKEFOURCC('B', 'C', '4', 'S');
const uint32 FOURCC_BC5S = MAKEFOURCC('B', 'C', '5', 'S');

const uint32 FOURCC_RGBG = MAKEFOURCC('R', 'G', 'B', 'G');
const uint32 FOURCC_GRGB = MAKEFOURCC('G', 'R', 'G', 'B');

const uint32 FOURCC_ATC_RGB = MAKEFOURCC('A', 'T', 'C', ' ');
const uint32 FOURCC_ATC_RGBA_EXPLICIT_ALPHA = MAKEFOURCC('A', 'T', 'C', 'I');
const uint32 FOURCC_ATC_RGBA_INTERPOLATED_ALPHA = MAKEFOURCC('A', 'T', 'C', 'A');

const uint32 FOURCC_A2XY = MAKEFOURCC('A', '2', 'X', 'Y');

const uint32 FOURCC_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y');
const uint32 FOURCC_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2');

const uint32 FOURCC_DX10 = MAKEFOURCC('D', 'X', '1', '0');

const uint32 DDSD_CAPS = 0x1U;
const uint32 DDSD_HEIGHT = 0x2U;
const uint32 DDSD_WIDTH = 0x4U;
const uint32 DDSD_PITCH = 0x8U;
const uint32 DDSD_PIXELFORMAT = 0x1000U;
const uint32 DDSD_MIPMAPCOUNT = 0x20000U;
const uint32 DDSD_LINEARSIZE = 0x80000U;
const uint32 DDSD_DEPTH = 0x800000;

const uint32 DDSCAPS_COMPLEX = 0x00000008U;
const uint32 DDSCAPS_TEXTURE = 0x00001000U;
const uint32 DDSCAPS_MIPMAP = 0x00400000U;
const uint32 DDSCAPS2_VOLUME = 0x00200000U;
const uint32 DDSCAPS2_CUBEMAP = 0x00000200U;

const uint32 DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400U;
const uint32 DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800U;
const uint32 DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000U;
const uint32 DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000U;
const uint32 DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000U;
const uint32 DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000U;
const uint32 DDSCAPS2_CUBEMAP_ALL_FACES = 0x0000FC00U;

enum D3D_FORMAT : uint32
{
    // 32 bit RGB formats.
    D3DFMT_R8G8B8 = 20,
    D3DFMT_A8R8G8B8 = 21,
    D3DFMT_X8R8G8B8 = 22,
    D3DFMT_R5G6B5 = 23,
    D3DFMT_X1R5G5B5 = 24,
    D3DFMT_A1R5G5B5 = 25,
    D3DFMT_A4R4G4B4 = 26,
    D3DFMT_R3G3B2 = 27,
    D3DFMT_A8 = 28,
    D3DFMT_A8R3G3B2 = 29,
    D3DFMT_X4R4G4B4 = 30,
    D3DFMT_A2B10G10R10 = 31,
    D3DFMT_A8B8G8R8 = 32,
    D3DFMT_X8B8G8R8 = 33,
    D3DFMT_G16R16 = 34,
    D3DFMT_A2R10G10B10 = 35,

    D3DFMT_A16B16G16R16 = 36,

    // Palette formats.
    D3DFMT_A8P8 = 40,
    D3DFMT_P8 = 41,

    // Luminance formats.
    D3DFMT_L8 = 50,
    D3DFMT_A8L8 = 51,
    D3DFMT_A4L4 = 52,
    D3DFMT_L16 = 81,

    // Floating point formats
    D3DFMT_R16F = 111,
    D3DFMT_G16R16F = 112,
    D3DFMT_A16B16G16R16F = 113,
    D3DFMT_R32F = 114,
    D3DFMT_G32R32F = 115,
    D3DFMT_A32B32G32R32F = 116,
};

enum DXGI_FORMAT : uint32
{
    DXGI_FORMAT_UNKNOWN = 0,

    DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
    DXGI_FORMAT_R32G32B32A32_UINT = 3,
    DXGI_FORMAT_R32G32B32A32_SINT = 4,

    DXGI_FORMAT_R32G32B32_TYPELESS = 5,
    DXGI_FORMAT_R32G32B32_FLOAT = 6,
    DXGI_FORMAT_R32G32B32_UINT = 7,
    DXGI_FORMAT_R32G32B32_SINT = 8,

    DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM = 11,
    DXGI_FORMAT_R16G16B16A16_UINT = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM = 13,
    DXGI_FORMAT_R16G16B16A16_SINT = 14,

    DXGI_FORMAT_R32G32_TYPELESS = 15,
    DXGI_FORMAT_R32G32_FLOAT = 16,
    DXGI_FORMAT_R32G32_UINT = 17,
    DXGI_FORMAT_R32G32_SINT = 18,

    DXGI_FORMAT_R32G8X24_TYPELESS = 19,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,

    DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
    DXGI_FORMAT_R10G10B10A2_UNORM = 24,
    DXGI_FORMAT_R10G10B10A2_UINT = 25,

    DXGI_FORMAT_R11G11B10_FLOAT = 26,

    DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    DXGI_FORMAT_R8G8B8A8_UINT = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM = 31,
    DXGI_FORMAT_R8G8B8A8_SINT = 32,

    DXGI_FORMAT_R16G16_TYPELESS = 33,
    DXGI_FORMAT_R16G16_FLOAT = 34,
    DXGI_FORMAT_R16G16_UNORM = 35,
    DXGI_FORMAT_R16G16_UINT = 36,
    DXGI_FORMAT_R16G16_SNORM = 37,
    DXGI_FORMAT_R16G16_SINT = 38,

    DXGI_FORMAT_R32_TYPELESS = 39,
    DXGI_FORMAT_D32_FLOAT = 40,
    DXGI_FORMAT_R32_FLOAT = 41,
    DXGI_FORMAT_R32_UINT = 42,
    DXGI_FORMAT_R32_SINT = 43,

    DXGI_FORMAT_R24G8_TYPELESS = 44,
    DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,

    DXGI_FORMAT_R8G8_TYPELESS = 48,
    DXGI_FORMAT_R8G8_UNORM = 49,
    DXGI_FORMAT_R8G8_UINT = 50,
    DXGI_FORMAT_R8G8_SNORM = 51,
    DXGI_FORMAT_R8G8_SINT = 52,

    DXGI_FORMAT_R16_TYPELESS = 53,
    DXGI_FORMAT_R16_FLOAT = 54,
    DXGI_FORMAT_D16_UNORM = 55,
    DXGI_FORMAT_R16_UNORM = 56,
    DXGI_FORMAT_R16_UINT = 57,
    DXGI_FORMAT_R16_SNORM = 58,
    DXGI_FORMAT_R16_SINT = 59,

    DXGI_FORMAT_R8_TYPELESS = 60,
    DXGI_FORMAT_R8_UNORM = 61,
    DXGI_FORMAT_R8_UINT = 62,
    DXGI_FORMAT_R8_SNORM = 63,
    DXGI_FORMAT_R8_SINT = 64,
    DXGI_FORMAT_A8_UNORM = 65,

    DXGI_FORMAT_R1_UNORM = 66,

    DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,

    DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
    DXGI_FORMAT_G8R8_G8B8_UNORM = 69,

    DXGI_FORMAT_BC1_TYPELESS = 70,
    DXGI_FORMAT_BC1_UNORM = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB = 72,

    DXGI_FORMAT_BC2_TYPELESS = 73,
    DXGI_FORMAT_BC2_UNORM = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB = 75,

    DXGI_FORMAT_BC3_TYPELESS = 76,
    DXGI_FORMAT_BC3_UNORM = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB = 78,

    DXGI_FORMAT_BC4_TYPELESS = 79,
    DXGI_FORMAT_BC4_UNORM = 80,
    DXGI_FORMAT_BC4_SNORM = 81,

    DXGI_FORMAT_BC5_TYPELESS = 82,
    DXGI_FORMAT_BC5_UNORM = 83,
    DXGI_FORMAT_BC5_SNORM = 84,

    DXGI_FORMAT_B5G6R5_UNORM = 85,
    DXGI_FORMAT_B5G5R5A1_UNORM = 86,
    DXGI_FORMAT_B8G8R8A8_UNORM = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM = 88
};

enum DDS_PIXELFORMAT_FLAGS : uint32
{
    DDPF_ALPHAPIXELS = 0x1,
    DDPF_ALPHA = 0x2,
    DDPF_FOURCC = 0x4,
    DDPF_RGB = 0x40,
    DDPF_YUV = 0x200,
    DDPF_LUMINANCE = 0x20000,
    DDPF_NORMAL = 0x80000000 // Custom nv flag
};

#pragma pack(push, 1)

struct DDS_PIXELFORMAT
{
    uint32 size = 0;
    uint32 flags = 0;
    uint32 fourCC = 0;
    uint32 RGBBitCount = 0;
    uint32 RBitMask = 0;
    uint32 GBitMask = 0;
    uint32 BBitMask = 0;
    uint32 ABitMask = 0;
};

struct DDS_RESERVED //uint32 reserved1[11]
{
    uint32 reserved0 = 0;
    uint32 reserved1 = 0;
    uint32 reserved2 = 0;
    uint32 reserved3 = 0;
    uint32 reserved4 = 0;
    uint32 reserved5 = 0;
    uint32 davaPixelFormat = 0;
    uint32 crcTag = 0;
    uint32 crcValue = 0;
    uint32 reserved9 = 0;
    uint32 reserved10 = 0;
};

struct DDS_HEADER
{
    uint32 size = 0;
    uint32 flags = 0;
    uint32 height = 0;
    uint32 width = 0;
    uint32 pitchOrLinearSize = 0;
    uint32 depth = 0;
    uint32 mipMapCount = 0;
    DDS_RESERVED reserved;
    DDS_PIXELFORMAT format;
    uint32 caps = 0;
    uint32 caps2 = 0;
    uint32 caps3 = 0;
    uint32 caps4 = 0;
    uint32 reserved2 = 0;
};

struct DDS_HEADER_DXT10
{
    uint32 dxgiFormat = 0;
    uint32 resourceDimension = 0;
    uint32 miscFlag = 0;
    uint32 arraySize = 0;
    uint32 miscFlags2 = 0;
};

const uint32 ddsFaces[] = {
    DDSCAPS2_CUBEMAP_POSITIVEX,
    DDSCAPS2_CUBEMAP_NEGATIVEX,
    DDSCAPS2_CUBEMAP_POSITIVEY,
    DDSCAPS2_CUBEMAP_NEGATIVEY,
    DDSCAPS2_CUBEMAP_POSITIVEZ,
    DDSCAPS2_CUBEMAP_NEGATIVEZ
};

#pragma pack(pop)

} // namespace dds

namespace D3DUtils
{
uint32 GetFormatSizeInBits(dds::D3D_FORMAT format)
{
    switch (format)
    {
    case dds::D3DFMT_R3G3B2:
    case dds::D3DFMT_A8:
    case dds::D3DFMT_P8:
    case dds::D3DFMT_L8:
    case dds::D3DFMT_A4L4:
        return 8;

    case dds::D3DFMT_R5G6B5:
    case dds::D3DFMT_X1R5G5B5:
    case dds::D3DFMT_A1R5G5B5:
    case dds::D3DFMT_A4R4G4B4:
    case dds::D3DFMT_X4R4G4B4:
    case dds::D3DFMT_A8R3G3B2:
    case dds::D3DFMT_A8P8:
    case dds::D3DFMT_A8L8:
    case dds::D3DFMT_L16:
    case dds::D3DFMT_R16F:
        return 8 * 2;

    case dds::D3DFMT_R8G8B8:
        return 8 * 3;

    case dds::D3DFMT_A8R8G8B8:
    case dds::D3DFMT_A8B8G8R8:
    case dds::D3DFMT_X8R8G8B8:
    case dds::D3DFMT_X8B8G8R8:
    case dds::D3DFMT_A2B10G10R10:
    case dds::D3DFMT_A2R10G10B10:
    case dds::D3DFMT_G16R16:
    case dds::D3DFMT_G16R16F:
    case dds::D3DFMT_R32F:
        return 8 * 4;

    case dds::D3DFMT_A16B16G16R16:
    case dds::D3DFMT_A16B16G16R16F:
    case dds::D3DFMT_G32R32F:
        return 8 * 8;

    case dds::D3DFMT_A32B32G32R32F:
        return 8 * 16;

    default:
        DVASSERT(false, Format("undefined format: %d", format).c_str());
        return 0;
    }
}

void DirectConvertFromD3D(const uint8* srcData, Image* dstImage, dds::D3D_FORMAT srcFormat, PixelFormat dstFormat)
{
    DVASSERT(dstImage);
    DVASSERT(dstImage->format == dstFormat);

    const uint32& w = dstImage->width;
    const uint32& h = dstImage->height;

    uint32 srcPitch = w * GetFormatSizeInBits(srcFormat) / 8;
    uint32 dstPitch = w * PixelFormatDescriptor::GetPixelFormatSizeInBits(dstFormat) / 8;

    switch (srcFormat)
    {
    case dds::D3DFMT_A8R8G8B8:
    {
        DVASSERT(dstFormat == FORMAT_RGBA8888);
        ConvertDirect<BGRA8888, RGBA8888, ConvertBGRA8888toRGBA8888> convert;
        convert(srcData, w, h, srcPitch, dstImage->data, w, h, dstPitch);
        break;
    }
    case dds::D3DFMT_X8R8G8B8:
    {
        DVASSERT(dstFormat == FORMAT_RGB888);
        ConvertDirect<BGRA8888, RGB888, ConvertBGRA8888toRGB888> convert;
        convert(srcData, w, h, srcPitch, dstImage->data, w, h, dstPitch);
        break;
    }
    case dds::D3DFMT_A1R5G5B5:
    {
        DVASSERT(dstFormat == FORMAT_RGBA5551);
        ConvertDirect<uint16, uint16, ConvertBGRA5551toRGBA5551> convert;
        convert(srcData, w, h, srcPitch, dstImage->data, w, h, dstPitch);
        break;
    }
    case dds::D3DFMT_A4R4G4B4:
    {
        DVASSERT(dstFormat == FORMAT_RGBA4444);
        ConvertDirect<uint16, uint16, ConvertBGRA4444toRGBA4444> convert;
        convert(srcData, w, h, srcPitch, dstImage->data, w, h, dstPitch);
        break;
    }
    default:
        DVASSERT(false, Format("undefined format: %d", srcFormat).c_str());
        break;
    }
}

void DirectConvertToD3D(const uint8* srcData, uint32 w, uint32 h, uint8* dstData, PixelFormat srcFormat, dds::D3D_FORMAT dstFormat)
{
    DVASSERT(srcData);
    DVASSERT(dstData);

    uint32 srcPitch = w * PixelFormatDescriptor::GetPixelFormatSizeInBits(srcFormat) / 8;
    uint32 dstPitch = w * GetFormatSizeInBits(dstFormat) / 8;

    switch (dstFormat)
    {
    case dds::D3DFMT_A8R8G8B8:
    {
        DVASSERT(srcFormat == FORMAT_RGBA8888);
        ConvertDirect<RGBA8888, BGRA8888, ConvertRGBA8888toBGRA8888> convert;
        convert(srcData, w, h, srcPitch, dstData, w, h, dstPitch);
        return;
    }
    case dds::D3DFMT_X8R8G8B8:
    {
        DVASSERT(srcFormat == FORMAT_RGB888);
        ConvertDirect<RGB888, BGRA8888, ConvertRGB888toBGRA8888> convert;
        convert(srcData, w, h, srcPitch, dstData, w, h, dstPitch);
        return;
    }
    case dds::D3DFMT_A1R5G5B5:
    {
        DVASSERT(srcFormat == FORMAT_RGBA5551);
        ConvertDirect<uint16, uint16, ConvertRGBA5551toARGB1555> convert;
        convert(srcData, w, h, srcPitch, dstData, w, h, dstPitch);
        return;
    }
    case dds::D3DFMT_A4R4G4B4:
    {
        DVASSERT(srcFormat == FORMAT_RGBA4444);
        ConvertDirect<uint16, uint16, ConvertRGBA4444toARGB4444> convert;
        convert(srcData, w, h, srcPitch, dstData, w, h, dstPitch);
        return;
    }
    default:
        DVASSERT(false, Format("undefined format: %d", dstFormat).c_str());
        break;
    }
}

void UpdatePitch(dds::DDS_HEADER& mainHeader, PixelFormat format)
{
    if (PixelFormatDescriptor::IsDxtFormat(format) || PixelFormatDescriptor::IsAtcFormat(format))
    {
        const PixelFormatDescriptor& formatDescriptor = PixelFormatDescriptor::GetPixelFormatDescriptor(format);
        const Size2i& blockSize = formatDescriptor.blockSize;

        uint32 blockSizeInBits = blockSize.dx * blockSize.dy * formatDescriptor.pixelSize;
        mainHeader.pitchOrLinearSize = (mainHeader.width + 3) / 4 * blockSizeInBits;
        mainHeader.flags |= dds::DDSD_LINEARSIZE;
    }
}

} // namespace D3DUtils

class DDSHandler
{
public:
    explicit DDSHandler(File* file);

    bool WriteMagicWord();
    bool WriteMainHeader();
    bool WriteExtHeader();
    bool WriteDataChunk(const uint8* data, uint32 size);
    bool WriteCRC(uint32 crc);

    bool ReadMagicWord();
    bool ReadHeaders();

    void SetFacesInfo();
    bool SetFormatInfo();

    void FetchFacesInfo();
    void FetchPixelFormats();

    bool IsSupportedFormat() const;

    File* file = nullptr;
    dds::DDS_HEADER mainHeader;
    dds::DDS_HEADER_DXT10 extHeader;
    bool fileStoresTextureInD3DFormat = false;
    dds::D3D_FORMAT d3dPixelFormat = dds::D3D_FORMAT::D3DFMT_R8G8B8;
    PixelFormat davaPixelFormat = FORMAT_INVALID;
    uint32 faceCount = 1;
    Array<rhi::TextureFace, Texture::CUBE_FACE_COUNT> faces;
};

DDSHandler::DDSHandler(File* _ddsFile)
    : file(_ddsFile)
{
}

bool DDSHandler::ReadMagicWord()
{
    uint32 dataRead = 0;
    if (sizeof(dds::FOURCC_DDS) == file->Read(&dataRead))
    {
        return dds::FOURCC_DDS == dataRead;
    }
    else
    {
        return false;
    }
}

bool DDSHandler::ReadHeaders()
{
    if (sizeof(dds::DDS_HEADER) != file->Read(&mainHeader))
    {
        return false;
    }

    if ((mainHeader.format.flags & dds::DDPF_FOURCC) && (mainHeader.format.fourCC == dds::FOURCC_DX10))
    {
        if (sizeof(dds::DDS_HEADER_DXT10) != file->Read(&extHeader))
        {
            return false;
        }
    }

    return true;
}

bool DDSHandler::WriteMagicWord()
{
    return (sizeof(dds::FOURCC_DDS) == file->Write(&dds::FOURCC_DDS, sizeof(dds::FOURCC_DDS)));
}

bool DDSHandler::WriteMainHeader()
{
    return (sizeof(mainHeader) == file->Write(&mainHeader, sizeof(mainHeader)));
}

bool DDSHandler::WriteExtHeader()
{
    return (mainHeader.format.fourCC == dds::FOURCC_DX10) ? (sizeof(extHeader) == file->Write(&extHeader, sizeof(extHeader))) : true;
}

bool DDSHandler::WriteCRC(uint32 crc)
{
    mainHeader.reserved.crcTag = FOURCC_CRC;
    mainHeader.reserved.crcValue = crc;

    file->Seek(0, File::SEEK_FROM_START);

    if (!WriteMagicWord() || !WriteMainHeader())
    {
        Logger::Error("Cannot write to file %s", file->GetFilename().GetStringValue().c_str());
        return false;
    }

    return true;
}

bool DDSHandler::WriteDataChunk(const uint8* data, uint32 dataSize)
{
    return (dataSize == file->Write(data, dataSize));
}

bool DDSHandler::IsSupportedFormat() const
{
    return (davaPixelFormat != FORMAT_INVALID);
}

void DDSHandler::SetFacesInfo()
{
    DVASSERT(faceCount <= Texture::CUBE_FACE_COUNT);

    if (faceCount == 1)
    {
        mainHeader.caps2 = 0;
    }
    else if (faceCount == Texture::CUBE_FACE_COUNT)
    {
        mainHeader.caps2 = dds::DDSCAPS2_CUBEMAP_ALL_FACES;
    }
    else
    {
        for (uint32 i = 0; i < faceCount; ++i)
        {
            mainHeader.caps2 |= dds::ddsFaces[i];
        }
    }
}

void DDSHandler::FetchFacesInfo()
{
    faceCount = 0;
    for (int face = 0; face < Texture::CUBE_FACE_COUNT; ++face)
    {
        if (mainHeader.caps2 & dds::ddsFaces[face])
        {
            faces[faceCount++] = static_cast<rhi::TextureFace>(face);
        }
    }

    if (faceCount == 0)
    {
        faceCount = 1;
    }
}

bool DDSHandler::SetFormatInfo()
{
    uint32& flags = mainHeader.format.flags;
    uint32& bitCount = mainHeader.format.RGBBitCount;
    uint32& rMask = mainHeader.format.RBitMask;
    uint32& gMask = mainHeader.format.GBitMask;
    uint32& bMask = mainHeader.format.BBitMask;
    uint32& aMask = mainHeader.format.ABitMask;
    uint32& fourcc = mainHeader.format.fourCC;

    mainHeader.format.size = sizeof(dds::DDS_PIXELFORMAT);

    fileStoresTextureInD3DFormat = false;

    switch (davaPixelFormat)
    {
    case FORMAT_RGBA8888:
    {
        flags = dds::DDPF_RGB | dds::DDPF_ALPHAPIXELS;
        rMask = 0xff;
        gMask = 0xff00;
        bMask = 0xff0000;
        aMask = 0xff000000;
        bitCount = 32;
        break;
    }
    case FORMAT_RGBA5551:
    {
        flags = dds::DDPF_RGB | dds::DDPF_ALPHAPIXELS;
        rMask = 0x7c00;
        gMask = 0x3e0;
        bMask = 0x1f;
        aMask = 0x8000;
        bitCount = 16;
        d3dPixelFormat = dds::D3DFMT_A1R5G5B5;
        fileStoresTextureInD3DFormat = true;
        break;
    }
    case FORMAT_RGBA4444:
    {
        flags = dds::DDPF_RGB | dds::DDPF_ALPHAPIXELS;
        rMask = 0xf00;
        gMask = 0xf0;
        bMask = 0xf;
        aMask = 0xf000;
        bitCount = 16;
        d3dPixelFormat = dds::D3DFMT_A4R4G4B4;
        fileStoresTextureInD3DFormat = true;
        break;
    }
    case FORMAT_RGB565:
    {
        flags = dds::DDPF_RGB;
        rMask = 0xf800;
        gMask = 0x7e0;
        bMask = 0x1f;
        bitCount = 16;
        break;
    }
    case FORMAT_RGB888:
    {
        flags = dds::DDPF_RGB;
        rMask = 0xff0000;
        gMask = 0xff00;
        bMask = 0xff;
        bitCount = 24;
        break;
    }
    case FORMAT_A8:
    {
        flags = dds::DDPF_ALPHA;
        bitCount = 8;
        aMask = 0xff;
        break;
    }
    case FORMAT_DXT1:
    case FORMAT_DXT1A:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::FOURCC_DXT1;
        break;
    }
    case FORMAT_DXT3:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::FOURCC_DXT3;
        break;
    }
    case FORMAT_DXT5:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::FOURCC_DXT5;
        break;
    }
    case FORMAT_DXT5NM:
    {
        flags = dds::DDPF_FOURCC | dds::DDPF_NORMAL;
        fourcc = dds::FOURCC_DXT5;
        break;
    }
    case FORMAT_ATC_RGB:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::FOURCC_ATC_RGB;
        break;
    }
    case FORMAT_ATC_RGBA_EXPLICIT_ALPHA:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::FOURCC_ATC_RGBA_EXPLICIT_ALPHA;
        break;
    }
    case FORMAT_ATC_RGBA_INTERPOLATED_ALPHA:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::FOURCC_ATC_RGBA_INTERPOLATED_ALPHA;
        break;
    }
    case FORMAT_R16F:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::D3DFMT_R16F;
        break;
    }
    case FORMAT_RG16F:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::D3DFMT_G16R16F;
        break;
    }
    case FORMAT_RGBA16F:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::D3DFMT_A16B16G16R16F;
        break;
    }
    case FORMAT_R32F:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::D3DFMT_R32F;
        break;
    }
    case FORMAT_RG32F:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::D3DFMT_G32R32F;
        break;
    }
    case FORMAT_RGBA32F:
    {
        flags = dds::DDPF_FOURCC;
        fourcc = dds::D3DFMT_A32B32G32R32F;
        break;
    }
    default:
    {
        return false;
    }
    }

    return true;
}

void DDSHandler::FetchPixelFormats()
{
    const uint32& flags = mainHeader.format.flags;
    const uint32& bitCount = mainHeader.format.RGBBitCount;
    const uint32& rMask = mainHeader.format.RBitMask;
    const uint32& gMask = mainHeader.format.GBitMask;
    const uint32& bMask = mainHeader.format.BBitMask;
    const uint32& aMask = mainHeader.format.ABitMask;
    bool flagRGB = ((flags & dds::DDPF_RGB) && !(flags & dds::DDPF_ALPHAPIXELS));
    bool flagRGBA = ((flags & dds::DDPF_RGB) && (flags & dds::DDPF_ALPHAPIXELS));
    bool flagAlpha = (flags & dds::DDPF_ALPHA) != 0;
    bool flagFourCC = (flags & dds::DDPF_FOURCC) != 0;

    fileStoresTextureInD3DFormat = false;
    davaPixelFormat = FORMAT_INVALID;

    if (flagRGBA)
    {
        if (rMask == 0xff && gMask == 0xff00 && bMask == 0xff0000 && aMask == 0xff000000)
        {
            davaPixelFormat = FORMAT_RGBA8888;
        }
        else if (rMask == 0x7c00 && gMask == 0x3e0 && bMask == 0x1f && aMask == 0x8000)
        {
            fileStoresTextureInD3DFormat = true;
            d3dPixelFormat = dds::D3DFMT_A1R5G5B5;
            davaPixelFormat = FORMAT_RGBA5551;
        }
        else if (rMask == 0xff0000 && gMask == 0xff00 && bMask == 0xff && aMask == 0xff000000)
        {
            fileStoresTextureInD3DFormat = true;
            d3dPixelFormat = dds::D3DFMT_A8R8G8B8;
            davaPixelFormat = FORMAT_RGBA8888;
        }
        else if (rMask == 0xf00 && gMask == 0xf0 && bMask == 0xf && aMask == 0xf000)
        {
            fileStoresTextureInD3DFormat = true;
            d3dPixelFormat = dds::D3DFMT_A4R4G4B4;
            davaPixelFormat = FORMAT_RGBA4444;
        }
    }
    else if (flagRGB)
    {
        if (rMask == 0xf800 && gMask == 0x7e0 && bMask == 0x1f)
        {
            davaPixelFormat = FORMAT_RGB565;
        }
        else if (rMask == 0xff0000 && gMask == 0xff00 && bMask == 0xff)
        {
            if (bitCount == 24)
            {
                davaPixelFormat = FORMAT_RGB888;
            }
            else if (bitCount == 32)
            {
                fileStoresTextureInD3DFormat = true;
                d3dPixelFormat = dds::D3DFMT_X8R8G8B8;
                davaPixelFormat = FORMAT_RGB888;
            }
        }
    }
    else if (flagAlpha && bitCount == 8 && aMask == 0xff)
    {
        davaPixelFormat = FORMAT_A8;
    }
    else if (flagFourCC)
    {
        if (mainHeader.format.fourCC == dds::FOURCC_DX10)
        {
            switch (extHeader.dxgiFormat)
            {
            case dds::DXGI_FORMAT_R8G8B8A8_UNORM:
            {
                davaPixelFormat = FORMAT_RGBA8888;
                break;
            }
            case dds::DXGI_FORMAT_B5G5R5A1_UNORM:
            {
                fileStoresTextureInD3DFormat = true;
                d3dPixelFormat = dds::D3DFMT_A1R5G5B5;
                davaPixelFormat = FORMAT_RGBA5551;
                break;
            }
            case dds::DXGI_FORMAT_B5G6R5_UNORM:
            {
                davaPixelFormat = FORMAT_RGB565;
                break;
            }
            case dds::DXGI_FORMAT_BC1_UNORM:
            {
                davaPixelFormat = FORMAT_DXT1;
                break;
            }
            case dds::DXGI_FORMAT_BC2_UNORM:
            {
                davaPixelFormat = FORMAT_DXT3;
                break;
            }
            case dds::DXGI_FORMAT_BC3_UNORM:
            {
                davaPixelFormat = (flags & dds::DDPF_NORMAL) ? FORMAT_DXT5NM : FORMAT_DXT5;
                break;
            }
            case dds::DXGI_FORMAT_R16G16B16A16_FLOAT:
            {
                davaPixelFormat = FORMAT_RGBA16F;
                break;
            }
            case dds::DXGI_FORMAT_R32G32B32A32_FLOAT:
            {
                davaPixelFormat = FORMAT_RGBA32F;
                break;
            }
            }
        }
        else
        {
            const uint32& fourcc = mainHeader.format.fourCC;
            if (fourcc == dds::FOURCC_DXT1)
            {
                davaPixelFormat = FORMAT_DXT1;
            }
            else if (fourcc == dds::FOURCC_DXT3)
            {
                davaPixelFormat = FORMAT_DXT3;
            }
            else if (fourcc == dds::FOURCC_DXT5)
            {
                davaPixelFormat = (flags & dds::DDPF_NORMAL) ? FORMAT_DXT5NM : FORMAT_DXT5;
            }
            else if (fourcc == dds::FOURCC_ATC_RGB)
            {
                davaPixelFormat = FORMAT_ATC_RGB;
            }
            else if (fourcc == dds::FOURCC_ATC_RGBA_EXPLICIT_ALPHA)
            {
                davaPixelFormat = FORMAT_ATC_RGBA_EXPLICIT_ALPHA;
            }
            else if (fourcc == dds::FOURCC_ATC_RGBA_INTERPOLATED_ALPHA)
            {
                davaPixelFormat = FORMAT_ATC_RGBA_INTERPOLATED_ALPHA;
            }
            else if (fourcc == dds::D3DFMT_R16F)
            {
                davaPixelFormat = FORMAT_R16F;
            }
            else if (fourcc == dds::D3DFMT_G16R16F)
            {
                davaPixelFormat = FORMAT_RG16F;
            }
            else if (fourcc == dds::D3DFMT_A16B16G16R16F)
            {
                davaPixelFormat = FORMAT_RGBA16F;
            }
            else if (fourcc == dds::D3DFMT_R32F)
            {
                davaPixelFormat = FORMAT_R32F;
            }
            else if (fourcc == dds::D3DFMT_G32R32F)
            {
                davaPixelFormat = FORMAT_RG32F;
            }
            else if (fourcc == dds::D3DFMT_A32B32G32R32F)
            {
                davaPixelFormat = FORMAT_RGBA32F;
            }
        }
    }

    if (davaPixelFormat == FORMAT_DXT1) // can actually be DXT1A
    {
        PixelFormat storedFormat = static_cast<PixelFormat>(mainHeader.reserved.davaPixelFormat);
        if (storedFormat == FORMAT_DXT1 || storedFormat == FORMAT_DXT1A)
        {
            davaPixelFormat = storedFormat;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

struct DDSReaderImpl : public DDSReader, public DDSHandler
{
    explicit DDSReaderImpl(File* file)
        : DDSHandler(file)
    {
    }

    // from DDSReader
    ImageInfo GetImageInfo() override;
    bool GetImages(Vector<Image*>& images, const ImageSystem::LoadingParams& loadingParams) override;
    bool GetCRC(uint32& crc) const override;
    bool AddCRC() override;
};

std::unique_ptr<DDSReader> DDSReader::CreateReader(File* file)
{
    DVASSERT(file != nullptr);

    DDSReaderImpl* readerImpl = new DDSReaderImpl(file);
    std::unique_ptr<DDSReader> ddsFile(readerImpl);
    DVASSERT(file->GetPos() == 0, Format("File %s position is not 0 (file had been already read)", file->GetFilename().GetStringValue().c_str()).c_str());

    if (readerImpl->ReadMagicWord() && readerImpl->ReadHeaders())
    {
        return ddsFile;
    }
    else
    {
        file->Seek(0, File::SEEK_FROM_START);
        return std::unique_ptr<DDSReader>(nullptr);
    }
}

ImageInfo DDSReaderImpl::GetImageInfo()
{
    ImageInfo info;

    FetchPixelFormats();
    FetchFacesInfo();

    if (IsSupportedFormat())
    {
        info.width = mainHeader.width;
        info.height = mainHeader.height;

        info.format = davaPixelFormat;

        uint32 headersSize = sizeof(dds::FOURCC_DDS) + sizeof(dds::DDS_HEADER);
        if (mainHeader.format.fourCC == dds::FOURCC_DX10)
        {
            headersSize += sizeof(dds::DDS_HEADER_DXT10);
        }

        info.dataSize = static_cast<uint32>(file->GetSize()) - headersSize;
        info.mipmapsCount = Max(mainHeader.mipMapCount, 1u); // for dds, image without multiple mipmaps has mipcount = 0, for us - it's 1
        info.faceCount = faceCount;
    }

    return info;
}

bool DDSReaderImpl::AddCRC()
{
    FilePath filepath = file->GetFilename();
    if (mainHeader.reserved.crcTag == FOURCC_CRC)
    {
        Logger::FrameworkDebug("CRC is already added into %s", filepath.GetStringValue().c_str());
        return false;
    }

    if (mainHeader.reserved.crcTag != 0 || mainHeader.reserved.crcValue != 0)
    {
        Logger::Warning("Reserved for CRC place is used in %s", filepath.GetStringValue().c_str());
        return false;
    }

    const uint32 fileSize = static_cast<uint32>(file->GetSize());
    Vector<char8> fileBuffer(fileSize);

    file->Seek(0, File::SEEK_FROM_START);
    if (file->Read(fileBuffer.data(), fileSize) != fileSize)
    {
        Logger::Error("Cannot read from file %s", filepath.GetStringValue().c_str());
        return false;
    }

    return WriteCRC(CRC32::ForBuffer(fileBuffer.data(), fileSize));
}

bool DDSReaderImpl::GetCRC(uint32& crc) const
{
    if (mainHeader.reserved.crcTag == FOURCC_CRC)
    {
        crc = mainHeader.reserved.crcValue;
        return true;
    }
    else
    {
        return false;
    }
}

bool DDSReaderImpl::GetImages(Vector<Image*>& images, const ImageSystem::LoadingParams& loadingParams)
{
    ImageInfo info = GetImageInfo();
    if (info.format == FORMAT_INVALID)
    {
        Logger::Error("Invalid/unsupported format of DDS file '%s'", file->GetFilename().GetStringValue().c_str());
        return false;
    }
    if (0 == info.width || 0 == info.height || 0 == info.mipmapsCount)
    {
        Logger::Error("Wrong mipmapsCount/width/height value for DDS file '%s'", file->GetFilename().GetStringValue().c_str());
        return false;
    }

    ImageSystem::LoadingParams ddsLoadingParams = { info.width, info.height, Min(loadingParams.baseMipmap, info.mipmapsCount - 1), loadingParams.firstMipmapIndex };
    uint32 baseMipMap = ImageSystem::GetBaseMipmap(ddsLoadingParams, loadingParams);

    const bool d3dUsed = fileStoresTextureInD3DFormat;
    const uint32 d3dBitsPerPixel = d3dUsed ? D3DUtils::GetFormatSizeInBits(d3dPixelFormat) : 0;
    const PixelFormat davaFormat = davaPixelFormat;

    auto MipSizeD3D = [d3dBitsPerPixel](uint32 w, uint32 h)
    {
        return w * h * d3dBitsPerPixel / 8;
    };
    auto MipSize = [d3dUsed, davaFormat, &MipSizeD3D](uint32 w, uint32 h)
    {
        return d3dUsed ? MipSizeD3D(w, h) : ImageUtils::GetSizeInBytes(w, h, davaFormat);
    };

    const uint32 largestImageSize = MipSize(info.width, info.height);
    Vector<uint8> dataBuffer(largestImageSize);

    for (uint32 faceIndex = 0; faceIndex < faceCount; ++faceIndex)
    {
        uint32 faceId = (faceCount > 1 ? faces[faceIndex] : Texture::INVALID_CUBEMAP_FACE);

        uint32 bytesToSkip = 0;
        for (uint32 mip = 0; mip < baseMipMap; ++mip)
        {
            uint32 mipWidth = Max(info.width >> mip, 1u);
            uint32 mipHeight = Max(info.height >> mip, 1u);
            bytesToSkip += MipSize(mipWidth, mipHeight);
        }
        if (bytesToSkip > 0 && (file->Seek(bytesToSkip, File::SEEK_FROM_CURRENT) == false))
        {
            Logger::Error("Can't seek in %s", file->GetFilename().GetStringValue().c_str());
            return false;
        }

        for (uint32 mip = baseMipMap; mip < info.mipmapsCount; ++mip)
        {
            uint32 mipWidth = Max(info.width >> mip, 1u);
            uint32 mipHeight = Max(info.height >> mip, 1u);
            uint32 bytesInMip = MipSize(mipWidth, mipHeight);

            auto readSize = file->Read(dataBuffer.data(), bytesInMip);
            if (readSize != bytesInMip)
            {
                Logger::Error("Can't read data from %s", file->GetFilename().GetStringValue().c_str());
                return false;
            }

            Image* fetchedImage = Image::Create(mipWidth, mipHeight, info.format);
            DVASSERT(fetchedImage);

            if (fileStoresTextureInD3DFormat)
            {
                D3DUtils::DirectConvertFromD3D(dataBuffer.data(), fetchedImage, d3dPixelFormat, davaPixelFormat);
            }
            else
            {
                Memcpy(fetchedImage->data, dataBuffer.data(), fetchedImage->dataSize);
            }

            fetchedImage->mipmapLevel = mip - baseMipMap + ddsLoadingParams.firstMipmapIndex;
            fetchedImage->cubeFaceID = faceId;

            images.push_back(fetchedImage);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////

struct DDSWriterImpl : public DDSWriter, public DDSHandler
{
    explicit DDSWriterImpl(File* file);

    // from DDSWriter
    bool Write(const Vector<Vector<Image*>>& images, PixelFormat dstFormat) override;
};

std::unique_ptr<DDSWriter> DDSWriter::CreateWriter(File* file)
{
    DVASSERT(file);
    DVASSERT(file->GetSize() == 0, Format("File '%s' is not empty", file->GetFilename().GetAbsolutePathname().c_str()).c_str());
    std::unique_ptr<DDSWriter> writerPtr(new DDSWriterImpl(file));
    return writerPtr;
}

DDSWriterImpl::DDSWriterImpl(File* file)
    : DDSHandler(file)
{
    Memset(&mainHeader, 0, sizeof(mainHeader));
}

bool DDSWriterImpl::Write(const Vector<Vector<Image*>>& images, PixelFormat dstFormat)
{
    DVASSERT(images.empty() == false);
    DVASSERT(images[0].empty() == false);
    DVASSERT(images[0][0] != nullptr);

    uint32 numImagesInFace = static_cast<uint32>(images[0].size());

    mainHeader.size = sizeof(dds::DDS_HEADER);
    mainHeader.mipMapCount = 0;
    mainHeader.flags = dds::DDSD_CAPS | dds::DDSD_WIDTH | dds::DDSD_HEIGHT | dds::DDSD_PIXELFORMAT;
    mainHeader.width = images[0][0]->width;
    mainHeader.height = images[0][0]->height;
    mainHeader.caps = dds::DDSCAPS_TEXTURE;

    D3DUtils::UpdatePitch(mainHeader, dstFormat);

    if (numImagesInFace > 1)
    {
        mainHeader.mipMapCount = static_cast<uint32>(numImagesInFace);
        mainHeader.flags |= dds::DDSD_MIPMAPCOUNT;
        mainHeader.caps |= (dds::DDSCAPS_COMPLEX | dds::DDSCAPS_MIPMAP);
    }

    PixelFormat srcFormat = images[0][0]->format;

    faceCount = 0;
    for (const Vector<Image*>& faceImageSet : images)
    {
        DVASSERT(static_cast<uint32>(faceImageSet.size()) == numImagesInFace, "image faces have different mips count");
        DVASSERT(faceImageSet[0] != nullptr);
        faces[faceCount++] = static_cast<rhi::TextureFace>(faceImageSet[0]->cubeFaceID);
        for (const Image* image : faceImageSet)
        {
            DVASSERT(image != nullptr, "image is null in set");
            DVASSERT(image->format == srcFormat, "image mips have different formats");
        }
    }

    SetFacesInfo();

    mainHeader.reserved.davaPixelFormat = davaPixelFormat = dstFormat;

    if (!SetFormatInfo())
    {
        Logger::Error("Format %s is not supported for writing into DDS", GlobalEnumMap<PixelFormat>::Instance()->ToString(dstFormat));
        return false;
    }

    if (!WriteMagicWord() || !WriteMainHeader() || !WriteExtHeader())
    {
        Logger::Error("Can't write header data");
        return false;
    }

    for (const Vector<Image*>& faceImageSet : images)
    {
        for (Image* mipImage : faceImageSet)
        {
            ScopedPtr<Image> dstDavaImage(nullptr);
            if (srcFormat != dstFormat)
            {
                dstDavaImage.reset(Image::Create(mipImage->width, mipImage->height, dstFormat)); // todo: use single image buffer for all mips
                if (ImageConvert::ConvertImage(mipImage, dstDavaImage) == false)
                {
                    Logger::Error("Can't convert from %s to %s",
                                  GlobalEnumMap<PixelFormat>::Instance()->ToString(srcFormat),
                                  GlobalEnumMap<PixelFormat>::Instance()->ToString(dstFormat));
                    return false;
                }
            }
            else
            {
                mipImage->Retain();
                dstDavaImage.reset(mipImage);
            }

            uint32 dataToCopySize = 0;
            uint8* dataToCopy = nullptr;
            Vector<uint8> d3dData; // todo: use single buffer for all mips
            if (fileStoresTextureInD3DFormat)
            {
                dataToCopySize = mipImage->width * mipImage->height * D3DUtils::GetFormatSizeInBits(d3dPixelFormat) / 8;
                d3dData.resize(dataToCopySize);
                D3DUtils::DirectConvertToD3D(dataToCopy, mipImage->width, mipImage->height, d3dData.data(), davaPixelFormat, d3dPixelFormat);
                dataToCopy = d3dData.data();
            }
            else
            {
                dataToCopy = dstDavaImage->data;
                dataToCopySize = dstDavaImage->dataSize;
            }

            if (WriteDataChunk(dataToCopy, dataToCopySize) == false)
            {
                Logger::Error("Can't add data chunk to %s", file->GetFilename().GetStringValue().c_str());
                return false;
            }
        }
    }

    return true;
}

} // namespace DAVA
