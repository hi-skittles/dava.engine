#include "Render/PixelFormatDescriptor.h"
#include "Render/Image/Image.h"
#include "Utils/Utils.h"
#include "Render/Renderer.h"

namespace DAVA
{
rhi::TextureFormat PixelFormatDescriptor::TEXTURE_FORMAT_INVALID = rhi::TextureFormat(-1);

UnorderedMap<PixelFormat, PixelFormatDescriptor, std::hash<uint8>> PixelFormatDescriptor::pixelDescriptors = {

    { FORMAT_INVALID, { FORMAT_INVALID, FastName("Invalid"), 0, TEXTURE_FORMAT_INVALID, false, false, Size2i(0, 0) } },

    { FORMAT_RGBA8888, { FORMAT_RGBA8888, FastName("RGBA8888"), 32, rhi::TEXTURE_FORMAT_R8G8B8A8, false, false, Size2i(1, 1) } },
    { FORMAT_RGBA5551, { FORMAT_RGBA5551, FastName("RGBA5551"), 16, rhi::TEXTURE_FORMAT_R5G5B5A1, false, false, Size2i(1, 1) } },
    { FORMAT_RGBA4444, { FORMAT_RGBA4444, FastName("RGBA4444"), 16, rhi::TEXTURE_FORMAT_R4G4B4A4, false, false, Size2i(1, 1) } },
    { FORMAT_RGB888, { FORMAT_RGB888, FastName("RGB888"), 24, rhi::TEXTURE_FORMAT_R8G8B8, false, false, Size2i(1, 1) } },
    { FORMAT_RGB565, { FORMAT_RGB565, FastName("RGB565"), 16, rhi::TEXTURE_FORMAT_R5G6B5, false, false, Size2i(1, 1) } },

    { FORMAT_A8, { FORMAT_A8, FastName("A8"), 8, rhi::TEXTURE_FORMAT_R8, false, false, Size2i(1, 1) } },
    { FORMAT_A16, { FORMAT_A16, FastName("A16"), 16, rhi::TEXTURE_FORMAT_R16, false, false, Size2i(1, 1) } },

    { FORMAT_RGBA16161616, { FORMAT_RGBA16161616, FastName("RGBA16161616"), 64, rhi::TEXTURE_FORMAT_A16R16G16B16, false, false, Size2i(1, 1) } },
    { FORMAT_RGBA32323232, { FORMAT_RGBA32323232, FastName("RGBA32323232"), 128, rhi::TEXTURE_FORMAT_A32R32G32B32, false, false, Size2i(1, 1) } },

    { FORMAT_PVR4, { FORMAT_PVR4, FastName("PVR4"), 4, rhi::TEXTURE_FORMAT_PVRTC_4BPP_RGBA, false, true, Size2i(8, 8) } },
    { FORMAT_PVR2, { FORMAT_PVR2, FastName("PVR2"), 2, rhi::TEXTURE_FORMAT_PVRTC_2BPP_RGBA, false, true, Size2i(16, 8) } },

    { FORMAT_DXT1, { FORMAT_DXT1, FastName("DXT1"), 4, rhi::TEXTURE_FORMAT_DXT1, false, true, Size2i(4, 4) } },
    { FORMAT_DXT1A, { FORMAT_DXT1A, FastName("DXT1a"), 4, rhi::TEXTURE_FORMAT_DXT1, false, true, Size2i(4, 4) } },
    { FORMAT_DXT3, { FORMAT_DXT3, FastName("DXT3"), 8, rhi::TEXTURE_FORMAT_DXT3, false, true, Size2i(4, 4) } },
    { FORMAT_DXT5, { FORMAT_DXT5, FastName("DXT5"), 8, rhi::TEXTURE_FORMAT_DXT5, false, true, Size2i(4, 4) } },
    { FORMAT_DXT5NM, { FORMAT_DXT5NM, FastName("DXT5nm"), 8, rhi::TEXTURE_FORMAT_DXT5, false, true, Size2i(4, 4) } },

    { FORMAT_ETC1, { FORMAT_ETC1, FastName("ETC1"), 4, rhi::TEXTURE_FORMAT_ETC1, false, true, Size2i(4, 4) } },

    { FORMAT_ATC_RGB, { FORMAT_ATC_RGB, FastName("ATC_RGB"), 4, rhi::TEXTURE_FORMAT_ATC_RGB, false, true, Size2i(4, 4) } },
    { FORMAT_ATC_RGBA_EXPLICIT_ALPHA, { FORMAT_ATC_RGBA_EXPLICIT_ALPHA, FastName("ATC_RGBA_EXPLICIT_ALPHA"), 8, rhi::TEXTURE_FORMAT_ATC_RGBA_EXPLICIT, false, true, Size2i(4, 4) } },
    { FORMAT_ATC_RGBA_INTERPOLATED_ALPHA, { FORMAT_ATC_RGBA_INTERPOLATED_ALPHA, FastName("ATC_RGBA_INTERPOLATED_ALPHA"), 8, rhi::TEXTURE_FORMAT_ATC_RGBA_INTERPOLATED, false, true, Size2i(4, 4) } },

    { FORMAT_PVR2_2, { FORMAT_PVR2_2, FastName("PVR2_2"), 2, rhi::TEXTURE_FORMAT_PVRTC2_2BPP_RGBA, false, true, Size2i(4, 4) } },
    { FORMAT_PVR4_2, { FORMAT_PVR4_2, FastName("PVR4_2"), 4, rhi::TEXTURE_FORMAT_PVRTC2_4BPP_RGBA, false, true, Size2i(8, 4) } },

    { FORMAT_EAC_R11_UNSIGNED, { FORMAT_EAC_R11_UNSIGNED, FastName("EAC_R11"), 8, rhi::TEXTURE_FORMAT_EAC_R11_UNSIGNED, false, true, Size2i(4, 4) } },
    { FORMAT_EAC_R11_SIGNED, { FORMAT_EAC_R11_SIGNED, FastName("EAC_R11_SIGNED"), 8, rhi::TEXTURE_FORMAT_EAC_R11_SIGNED, false, true, Size2i(4, 4) } },
    { FORMAT_EAC_RG11_UNSIGNED, { FORMAT_EAC_RG11_UNSIGNED, FastName("EAC_RG11"), 8, rhi::TEXTURE_FORMAT_EAC_R11G11_UNSIGNED, false, true, Size2i(4, 4) } },
    { FORMAT_EAC_RG11_SIGNED, { FORMAT_EAC_RG11_SIGNED, FastName("EAC_RG11_SIGNED"), 8, rhi::TEXTURE_FORMAT_EAC_R11G11_SIGNED, false, true, Size2i(4, 4) } },

    { FORMAT_ETC2_RGB, { FORMAT_ETC2_RGB, FastName("ETC2_RGB"), 4, rhi::TEXTURE_FORMAT_ETC2_R8G8B8, false, true, Size2i(4, 4) } },
    { FORMAT_ETC2_RGBA, { FORMAT_ETC2_RGBA, FastName("ETC2_RGBA"), 4, rhi::TEXTURE_FORMAT_ETC2_R8G8B8A8, false, true, Size2i(4, 4) } },
    { FORMAT_ETC2_RGB_A1, { FORMAT_ETC2_RGB_A1, FastName("ETC2_RGB_A1"), 4, rhi::TEXTURE_FORMAT_ETC2_R8G8B8A1, false, true, Size2i(4, 4) } }

#if defined(__DAVAENGINE_WIN32__)
    ,
    { FORMAT_BGR888, { FORMAT_BGR888, FastName("BGR888"), 24, TEXTURE_FORMAT_INVALID, false, false, Size2i(1, 1) } }
#endif

    ,
    { FORMAT_R16F, { FORMAT_R16F, FastName("R16F"), 16, rhi::TextureFormat::TEXTURE_FORMAT_R16F, false, false, Size2i(1, 1) } },
    { FORMAT_RG16F, { FORMAT_RG16F, FastName("RG16F"), 32, rhi::TextureFormat::TEXTURE_FORMAT_RG16F, false, false, Size2i(1, 1) } },
    { FORMAT_RGBA16F, { FORMAT_RGBA16F, FastName("RGBA16F"), 64, rhi::TextureFormat::TEXTURE_FORMAT_RGBA16F, false, false, Size2i(1, 1) } },

    { FORMAT_R32F, { FORMAT_R32F, FastName("R32F"), 32, rhi::TextureFormat::TEXTURE_FORMAT_R32F, false, false, Size2i(1, 1) } },
    { FORMAT_RG32F, { FORMAT_RG32F, FastName("RG32F"), 64, rhi::TextureFormat::TEXTURE_FORMAT_RG32F, false, false, Size2i(1, 1) } },
    { FORMAT_RGBA32F, { FORMAT_RGBA32F, FastName("RGBA32F"), 128, rhi::TextureFormat::TEXTURE_FORMAT_RGBA32F, false, false, Size2i(1, 1) } }
};

const PixelFormatDescriptor& PixelFormatDescriptor::GetPixelFormatDescriptor(const PixelFormat format)
{
    auto descrFound = pixelDescriptors.find(format);
    DVASSERT(descrFound != pixelDescriptors.end());
    return descrFound->second;
}

void PixelFormatDescriptor::SetHardwareSupportedFormats()
{
    for (auto& entry : pixelDescriptors)
    {
        PixelFormatDescriptor& descr = entry.second;
        if (descr.format != TEXTURE_FORMAT_INVALID)
        {
            descr.isHardwareSupported = rhi::TextureFormatSupported(descr.format);
        }
    }
}

const char* PixelFormatDescriptor::GetPixelFormatString(const PixelFormat format)
{
    return GetPixelFormatDescriptor(format).name.c_str();
}

PixelFormat PixelFormatDescriptor::GetPixelFormatByName(const FastName& formatName)
{
    for (const auto& entry : pixelDescriptors)
    {
        const PixelFormatDescriptor& descr = entry.second;
        if (formatName == descr.name)
        {
            return descr.formatID;
        }
    }

    return FORMAT_INVALID;
}

bool PixelFormatDescriptor::IsCompressedFormat(PixelFormat format)
{
    return GetPixelFormatDescriptor(format).isCompressed;
}

Size2i PixelFormatDescriptor::GetPixelFormatBlockSize(PixelFormat formatID)
{
    return GetPixelFormatDescriptor(formatID).blockSize;
}

bool PixelFormatDescriptor::IsFormatSizeByteDivisible(PixelFormat format)
{
    PixelFormatDescriptor descriptor = GetPixelFormatDescriptor(format);
    return (descriptor.pixelSize % 8 == 0 && descriptor.blockSize == Size2i(1, 1));
}

bool PixelFormatDescriptor::IsFloatPixelFormat(PixelFormat fmt)
{
    return (fmt == PixelFormat::FORMAT_R16F) || (fmt == PixelFormat::FORMAT_R32F) ||
    (fmt == PixelFormat::FORMAT_RG16F) || (fmt == PixelFormat::FORMAT_RG32F) ||
    (fmt == PixelFormat::FORMAT_RGB16F) || (fmt == PixelFormat::FORMAT_RGB32F) ||
    (fmt == PixelFormat::FORMAT_RGBA16F) || (fmt == PixelFormat::FORMAT_RGBA32F);
}

void PixelFormatDescriptor::GetFloatFormatInfo(uint32 width, PixelFormat format, uint32& channels, uint32& channelSize, uint32& pitch)
{
    static const DAVA::Map<DAVA::PixelFormat, std::pair<uint32, uint32>> mapping =
    {
      { FORMAT_R16F, { 1, 2 } },
      { FORMAT_R32F, { 1, 4 } },
      { FORMAT_RG16F, { 2, 2 } },
      { FORMAT_RG32F, { 2, 4 } },
      { FORMAT_RGB16F, { 3, 2 } },
      { FORMAT_RGB32F, { 3, 4 } },
      { FORMAT_RGBA16F, { 4, 2 } },
      { FORMAT_RGBA32F, { 4, 4 } },
    };
    auto i = mapping.find(format);
    DVASSERT(i != mapping.end(), "Unsupported input format supplied to GetFloatFormatInfo");
    channels = i->second.first;
    channelSize = i->second.second;
    pitch = ImageUtils::GetPitchInBytes(width, format);
}

bool PixelFormatDescriptor::IsDxtFormat(PixelFormat format)
{
    return (format >= FORMAT_DXT1 && format <= FORMAT_DXT5NM);
}

bool PixelFormatDescriptor::IsAtcFormat(PixelFormat format)
{
    return (format == FORMAT_ATC_RGB ||
            format == FORMAT_ATC_RGBA_EXPLICIT_ALPHA ||
            format == FORMAT_ATC_RGBA_INTERPOLATED_ALPHA);
}

bool PixelFormatDescriptor::IsPVRFormat(PixelFormat format)
{
    return (format == FORMAT_PVR2 || format == FORMAT_PVR4 || format == FORMAT_ETC1);
}
}
