#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/BaseObject.h"
#include "FileSystem/FilePath.h"

#include "Render/Image/ImageSystem.h"
#include "Render/Image/ImageFormatInterface.h"

namespace DAVA
{
class Texture;
class Sprite;
class Image;

class LibPngHelper : public ImageFormatInterface
{
public:
    LibPngHelper();

    static eErrorCode ReadPngFile(File* infile, Image* image, PixelFormat targetFormat = FORMAT_INVALID);

    eErrorCode ReadFile(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const override;
    eErrorCode WriteFile(const FilePath& fileName, const Vector<Image*>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const override;
    eErrorCode WriteFileAsCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const override;

    ImageInfo GetImageInfo(File* infile) const override;

protected:
    bool CanProcessFileInternal(File* infile) const override;
};
}
