#pragma once

#include "Render/Image/Image.h"
#include "Render/Image/ImageFormatInterface.h"
#include <memory>

namespace DAVA
{
class LibHDRHelper : public ImageFormatInterface
{
public:
    LibHDRHelper();

    ImageInfo GetImageInfo(File* infile) const override;

    eErrorCode ReadFile(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const override;
    eErrorCode WriteFile(const FilePath& fileName, const Vector<Image*>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const override;
    eErrorCode WriteFileAsCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const override;

private:
    ImageInfo GetImageInfo(File* infile, Size2i& flip) const;
    bool CanProcessFileInternal(File* file) const override;
};
}
