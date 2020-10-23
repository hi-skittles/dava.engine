#pragma once

#include "Base/BaseTypes.h"
#include "Render/RenderBase.h"
#include "FileSystem/FilePath.h"
#include "Render/Image/ImageFormatInterface.h"

namespace DAVA
{
class File;
class Image;

class LibDdsHelper : public ImageFormatInterface
{
public:
    LibDdsHelper();

    static bool AddCRCIntoMetaData(const FilePath& filePathname);
    static uint32 GetCRCFromMetaData(const FilePath& filePathname);

    eErrorCode ReadFile(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const override;

    eErrorCode WriteFile(const FilePath& fileName, const Vector<Image*>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const override;
    eErrorCode WriteFileAsCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const override;

    ImageInfo GetImageInfo(File* infile) const override;

protected:
    bool CanProcessFileInternal(File* infile) const override;

private:
    eErrorCode WriteFileInternal(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const;
};
};
