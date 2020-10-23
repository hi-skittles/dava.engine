#pragma once 

#include "Base/BaseTypes.h"
#include "Logger/Logger.h"
#include "Render/RenderBase.h"
#include "Render/RHI/rhi_Type.h"

namespace DAVA
{
/**
    \ingroup render
    \brief Class that represents pixel format for internal using in our SDK. 
 */

class PixelFormatDescriptor
{
public:
    static void SetHardwareSupportedFormats();

    static int32 GetPixelFormatSizeInBits(PixelFormat format);
    static Size2i GetPixelFormatBlockSize(PixelFormat format);

    /**
    returns true if `format` has pixel size of 8, 16 etc and block size is (1,1).
    Normally, uncompressed formats should return true while compressed formats should return false.
    Function is especially useful for pixel ariphmetics: is format is byte divisible you can safely
    operate with blocks of pixels, iterate over pixels etc.
    */
    static bool IsFormatSizeByteDivisible(PixelFormat format);

    static const char* GetPixelFormatString(const PixelFormat format);
    static PixelFormat GetPixelFormatByName(const FastName& formatName);

    static const PixelFormatDescriptor& GetPixelFormatDescriptor(const PixelFormat format);

    static bool IsCompressedFormat(PixelFormat format);

    static bool IsFloatPixelFormat(PixelFormat fmt);
    static void GetFloatFormatInfo(uint32 width, PixelFormat format, uint32& channels, uint32& channelSize, uint32& pitch);

    static bool IsDxtFormat(PixelFormat format);
    static bool IsAtcFormat(PixelFormat format);
    static bool IsPVRFormat(PixelFormat format);

private:
    static UnorderedMap<PixelFormat, PixelFormatDescriptor, std::hash<uint8>> pixelDescriptors;

public:
    static rhi::TextureFormat TEXTURE_FORMAT_INVALID;

    PixelFormat formatID; // compile-time: pixel format for DAVA::Image
    FastName name; // compile-time: name for logging/console tools
    uint8 pixelSize; // compile-time: size of pixel in bits
    rhi::TextureFormat format; // compile-time: pixel format for texture and rendering of the DAVA::Image
    bool isHardwareSupported; // run-time: is true for rhi::TextureFormat that are supported by GPU on every device
    bool isCompressed; // compile-time: is true for PVR2/4, DXT[n], ATC_[] formats
    Size2i blockSize; // compile-time: size of block for compressed formats, should be 1x1 for uncompressed formats
};

inline int32 PixelFormatDescriptor::GetPixelFormatSizeInBits(const PixelFormat format)
{
    return GetPixelFormatDescriptor(format).pixelSize;
}
}
