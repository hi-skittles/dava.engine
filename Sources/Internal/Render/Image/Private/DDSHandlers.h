#pragma once

#include "Base/BaseTypes.h"
#include "Render/Image/ImageFormatInterface.h"

namespace DAVA
{
class Image;

struct DDSReader;
struct DDSWriter;

struct DDSReader
{
    virtual ~DDSReader() = default;
    virtual ImageInfo GetImageInfo() = 0;
    virtual bool GetImages(Vector<Image*>& images, const ImageSystem::LoadingParams& loadingParams) = 0;
    virtual bool GetCRC(uint32& crc) const = 0;
    virtual bool AddCRC() = 0;

    static std::unique_ptr<DDSReader> CreateReader(File* file);
};

struct DDSWriter
{
    virtual ~DDSWriter() = default;
    virtual bool Write(const Vector<Vector<Image*>>& images, PixelFormat dstFormat) = 0;

    static std::unique_ptr<DDSWriter> CreateWriter(File* file);
};

} // namespace DAVA
