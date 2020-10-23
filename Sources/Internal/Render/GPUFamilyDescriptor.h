#ifndef __GPU_FAMILY_H__
#define __GPU_FAMILY_H__

#include "Base/BaseTypes.h"
#include "Render/RenderBase.h"

namespace DAVA
{
class FilePath;
class TextureDescriptor;

namespace GPUFamilyDescriptor
{
const Map<PixelFormat, ImageFormat>& GetAvailableFormatsForGpu(eGPUFamily gpuFamily);

const String& GetGPUName(const eGPUFamily gpuFamily);
const String& GetGPUPrefix(const eGPUFamily gpuFamily);
ImageFormat GetCompressedFileFormat(const eGPUFamily gpuFamily, const PixelFormat pixelFormat);

eGPUFamily GetGPUForPathname(const FilePath& pathname);
eGPUFamily GetGPUByName(const String& name);
DAVA_DEPRECATED(eGPUFamily ConvertValueToGPU(const int32 value));

bool IsGPUForDevice(const eGPUFamily gpu);
bool IsFormatSupported(const eGPUFamily gpu, const PixelFormat format);
}
}

#endif // __GPU_FAMILY_H__
