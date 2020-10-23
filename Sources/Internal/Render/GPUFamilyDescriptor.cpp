#include "Render/GPUFamilyDescriptor.h"
#include "Logger/Logger.h"
#include "Debug/DVAssert.h"
#include "FileSystem/FilePath.h"
#include "Utils/StringFormat.h"
#include "Utils/Utils.h"
#include "Render/TextureDescriptor.h"
#include "Render/Texture.h"
#include "Render/PixelFormatDescriptor.h"
#include "Render/Image/ImageSystem.h"

namespace DAVA
{
namespace GPUFamilyDescriptor
{
struct GPUData
{
    String name;
    String prefix;

    Map<PixelFormat, ImageFormat> availableFormats;
};

UnorderedMap<eGPUFamily, GPUData, std::hash<uint8>> gpuData =
{
  { GPU_POWERVR_IOS, { "PowerVR_iOS", ".PowerVR_iOS", {
                                                      { FORMAT_RGBA8888, IMAGE_FORMAT_PVR },
                                                      { FORMAT_RGBA5551, IMAGE_FORMAT_PVR },
                                                      { FORMAT_RGBA4444, IMAGE_FORMAT_PVR },
                                                      { FORMAT_RGB888, IMAGE_FORMAT_PVR },
                                                      { FORMAT_RGB565, IMAGE_FORMAT_PVR },
                                                      { FORMAT_A8, IMAGE_FORMAT_PVR },
                                                      { FORMAT_PVR4, IMAGE_FORMAT_PVR },
                                                      { FORMAT_PVR2, IMAGE_FORMAT_PVR },
                                                      { FORMAT_RGBA16F, IMAGE_FORMAT_PVR },
                                                      { FORMAT_RGBA32F, IMAGE_FORMAT_PVR }
                                                      } } },
  { GPU_POWERVR_ANDROID, { "PowerVR_Android", ".PowerVR_Android", {
                                                                  { FORMAT_RGBA8888, IMAGE_FORMAT_PVR },
                                                                  { FORMAT_RGBA5551, IMAGE_FORMAT_PVR },
                                                                  { FORMAT_RGBA4444, IMAGE_FORMAT_PVR },
                                                                  { FORMAT_RGB888, IMAGE_FORMAT_PVR },
                                                                  { FORMAT_RGB565, IMAGE_FORMAT_PVR },
                                                                  { FORMAT_A8, IMAGE_FORMAT_PVR },
                                                                  { FORMAT_PVR4, IMAGE_FORMAT_PVR },
                                                                  { FORMAT_PVR2, IMAGE_FORMAT_PVR },
                                                                  { FORMAT_ETC1, IMAGE_FORMAT_PVR },
                                                                  { FORMAT_RGBA16F, IMAGE_FORMAT_PVR },
                                                                  { FORMAT_RGBA32F, IMAGE_FORMAT_PVR }
                                                                  } } },
  { GPU_TEGRA, { "tegra", ".tegra", {
                                    { FORMAT_RGBA8888, IMAGE_FORMAT_PVR },
                                    { FORMAT_RGBA5551, IMAGE_FORMAT_PVR },
                                    { FORMAT_RGBA4444, IMAGE_FORMAT_PVR },
                                    { FORMAT_RGB888, IMAGE_FORMAT_PVR },
                                    { FORMAT_RGB565, IMAGE_FORMAT_PVR },
                                    { FORMAT_A8, IMAGE_FORMAT_PVR },
                                    { FORMAT_DXT1, IMAGE_FORMAT_DDS },
                                    { FORMAT_DXT1A, IMAGE_FORMAT_DDS },
                                    { FORMAT_DXT3, IMAGE_FORMAT_DDS },
                                    { FORMAT_DXT5, IMAGE_FORMAT_DDS },
                                    { FORMAT_DXT5NM, IMAGE_FORMAT_DDS },
                                    { FORMAT_ETC1, IMAGE_FORMAT_PVR },
                                    { FORMAT_RGBA16F, IMAGE_FORMAT_DDS },
                                    { FORMAT_RGBA32F, IMAGE_FORMAT_DDS }
                                    } } },
  { GPU_MALI, { "mali", ".mali", {
                                 { FORMAT_RGBA8888, IMAGE_FORMAT_PVR },
                                 { FORMAT_RGBA5551, IMAGE_FORMAT_PVR },
                                 { FORMAT_RGBA4444, IMAGE_FORMAT_PVR },
                                 { FORMAT_RGB888, IMAGE_FORMAT_PVR },
                                 { FORMAT_RGB565, IMAGE_FORMAT_PVR },
                                 { FORMAT_A8, IMAGE_FORMAT_PVR },
                                 { FORMAT_ETC1, IMAGE_FORMAT_PVR },
                                 { FORMAT_RGBA16F, IMAGE_FORMAT_PVR },
                                 { FORMAT_RGBA32F, IMAGE_FORMAT_PVR }
                                 } } },
  { GPU_ADRENO, { "adreno", ".adreno", {
                                       { FORMAT_RGBA8888, IMAGE_FORMAT_PVR },
                                       { FORMAT_RGBA5551, IMAGE_FORMAT_PVR },
                                       { FORMAT_RGBA4444, IMAGE_FORMAT_PVR },
                                       { FORMAT_RGB888, IMAGE_FORMAT_PVR },
                                       { FORMAT_RGB565, IMAGE_FORMAT_PVR },
                                       { FORMAT_A8, IMAGE_FORMAT_PVR },
                                       { FORMAT_ETC1, IMAGE_FORMAT_PVR },
                                       { FORMAT_ATC_RGB, IMAGE_FORMAT_DDS },
                                       { FORMAT_ATC_RGBA_EXPLICIT_ALPHA, IMAGE_FORMAT_DDS },
                                       { FORMAT_ATC_RGBA_INTERPOLATED_ALPHA, IMAGE_FORMAT_DDS },
                                       { FORMAT_RGBA16F, IMAGE_FORMAT_DDS },
                                       { FORMAT_RGBA32F, IMAGE_FORMAT_DDS }
                                       } } },
  { GPU_DX11, { "dx11", ".dx11", {
                                 { FORMAT_RGBA8888, IMAGE_FORMAT_PVR },
                                 { FORMAT_RGBA5551, IMAGE_FORMAT_PVR },
                                 { FORMAT_RGBA4444, IMAGE_FORMAT_PVR },
                                 { FORMAT_RGB565, IMAGE_FORMAT_PVR },
                                 { FORMAT_A8, IMAGE_FORMAT_PVR },
                                 { FORMAT_DXT1, IMAGE_FORMAT_DDS },
                                 { FORMAT_DXT1A, IMAGE_FORMAT_DDS },
                                 { FORMAT_DXT3, IMAGE_FORMAT_DDS },
                                 { FORMAT_DXT5, IMAGE_FORMAT_DDS },
                                 { FORMAT_DXT5NM, IMAGE_FORMAT_DDS },
                                 { FORMAT_RGBA16F, IMAGE_FORMAT_DDS },
                                 { FORMAT_RGBA32F, IMAGE_FORMAT_DDS }
                                 } } },
  { GPU_ORIGIN, { "origin", "", {
                                { FORMAT_RGBA8888, IMAGE_FORMAT_UNKNOWN },
                                { FORMAT_RGBA5551, IMAGE_FORMAT_UNKNOWN },
                                { FORMAT_RGB888, IMAGE_FORMAT_UNKNOWN },
                                { FORMAT_A8, IMAGE_FORMAT_UNKNOWN },
                                { FORMAT_A16, IMAGE_FORMAT_UNKNOWN },
                                { FORMAT_RGBA16F, IMAGE_FORMAT_UNKNOWN },
                                { FORMAT_RGBA32F, IMAGE_FORMAT_UNKNOWN }
                                } } }
};

const Map<PixelFormat, ImageFormat>& GetAvailableFormatsForGpu(eGPUFamily gpuFamily)
{
    DVASSERT(0 <= gpuFamily && gpuFamily < GPU_FAMILY_COUNT);

    return gpuData[gpuFamily].availableFormats;
}

eGPUFamily GetGPUForPathname(const FilePath& pathname)
{
    const String filename = pathname.GetFilename();

    for (const auto& it : gpuData)
    {
        if (String::npos != filename.rfind(it.second.prefix))
        {
            return it.first;
        }
    }

    const String ext = pathname.GetExtension();
    bool isUncompressed = TextureDescriptor::IsSourceTextureExtension(ext);
    return isUncompressed ? GPU_ORIGIN : GPU_INVALID;
}

const String& GetGPUName(const eGPUFamily gpuFamily)
{
    DVASSERT(0 <= gpuFamily && gpuFamily < GPU_FAMILY_COUNT);

    return gpuData[gpuFamily].name;
}

const String& GetGPUPrefix(const eGPUFamily gpuFamily)
{
    DVASSERT(0 <= gpuFamily && gpuFamily < GPU_FAMILY_COUNT);

    return gpuData[gpuFamily].prefix;
}

eGPUFamily GetGPUByName(const String& name)
{
    for (int32 i = 0; i < GPU_FAMILY_COUNT; ++i)
    {
        eGPUFamily gpu = eGPUFamily(i);
        if (name == gpuData[gpu].name)
        {
            return gpu;
        }
    }

    return GPU_INVALID;
}

bool IsFormatSupported(const eGPUFamily gpu, const PixelFormat format)
{
    if (gpu < 0 || gpu >= GPU_FAMILY_COUNT || format == FORMAT_INVALID)
    {
        return false;
    }
    return gpuData[gpu].availableFormats.find(format) != gpuData[gpu].availableFormats.end();
}

ImageFormat GetCompressedFileFormat(const eGPUFamily gpuFamily, const PixelFormat pixelFormat)
{
    if (!IsGPUForDevice(gpuFamily) || pixelFormat == FORMAT_INVALID)
        return IMAGE_FORMAT_UNKNOWN;

    auto& gpuFormats = gpuData[gpuFamily].availableFormats;
    auto formatFound = gpuFormats.find(pixelFormat);
    if (formatFound == gpuFormats.end())
    {
        Logger::Error("[GPUFamilyDescriptor::GetFileFormat: can't find pixel format %s for gpu %s]", PixelFormatDescriptor::GetPixelFormatString(pixelFormat), gpuData[gpuFamily].name.c_str());
        return IMAGE_FORMAT_UNKNOWN;
    }

    return formatFound->second;
}

eGPUFamily ConvertValueToGPU(const int32 value)
{
    if (value >= 0 && value < GPU_FAMILY_COUNT)
    {
        return static_cast<eGPUFamily>(value);
    }
    else if (value == -1) // -1 is an old value, left for compatibility: some old tex files may contain gpu = -1
    {
        return GPU_ORIGIN;
    }
    else
    {
        return GPU_INVALID;
    }
}

bool IsGPUForDevice(const eGPUFamily gpu)
{
    return (gpu >= 0 && gpu < GPU_DEVICE_COUNT);
}
}
}
