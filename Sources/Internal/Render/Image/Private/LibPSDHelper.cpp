#include "Render/Image/LibPSDHelper.h"
#include "Render/Image/Image.h"
#include "Render/PixelFormatDescriptor.h"
#include "FileSystem/File.h"

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
#include "libpsd/libpsd.h"
#endif

namespace DAVA
{
LibPSDHelper::LibPSDHelper()
    : ImageFormatInterface(ImageFormat::IMAGE_FORMAT_PSD, "PSD", { ".psd" }, { FORMAT_RGBA8888 })
{
}

eErrorCode LibPSDHelper::ReadFile(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const
{
#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
    auto fileName = infile->GetFilename().GetAbsolutePathname();
    psd_context* psd = nullptr;
    auto status = psd_image_load(&psd, const_cast<psd_char*>(fileName.c_str()));
    if ((psd == nullptr) || (status != psd_status_done))
    {
        DAVA::Logger::Error("============================ ERROR ============================");
        DAVA::Logger::Error("| Unable to load PSD from file (result code = %d):", static_cast<int>(status));
        DAVA::Logger::Error("| %s", fileName.c_str());
        DAVA::Logger::Error("| Try to re-save this file by using `Save as...` in Photoshop");
        DAVA::Logger::Error("===============================================================");
        return eErrorCode::ERROR_READ_FAIL;
    }

    SCOPE_EXIT
    {
        psd_image_free(psd);
    };

    imageSet.reserve(psd->layer_count);
    for (psd_short l = 0; l < psd->layer_count; ++l)
    {
        auto& layer = psd->layer_records[l];

        // swap R and B channels
        DAVA::uint32* p = reinterpret_cast<DAVA::uint32*>(layer.image_data);
        for (psd_int i = 0, e = layer.width * layer.height; i < e; ++i)
        {
            p[i] = (p[i] & 0xff00ff00) | (p[i] & 0x000000ff) << 16 | (p[i] & 0xff0000) >> 16;
        }
        ScopedPtr<Image> layerImage(Image::CreateFromData(layer.width, layer.height, DAVA::PixelFormat::FORMAT_RGBA8888,
                                                          reinterpret_cast<DAVA::uint8*>(layer.image_data)));

        Vector<uint8> emptyData(psd->width * psd->height * PixelFormatDescriptor::GetPixelFormatSizeInBits(DAVA::PixelFormat::FORMAT_RGBA8888) / 8, 0);
        Image* resultImage = Image::CreateFromData(psd->width, psd->height, DAVA::PixelFormat::FORMAT_RGBA8888, emptyData.data());
        resultImage->InsertImage(layerImage, layer.left, layer.top);
        resultImage->mipmapLevel = loadingParams.firstMipmapIndex;
        imageSet.push_back(resultImage);
    }

    return eErrorCode::SUCCESS;
#else
    Logger::Error("[LibPSDHelper::ReadFile] PSD reading is disabled for this platform");
    return eErrorCode::ERROR_READ_FAIL;
#endif
}

eErrorCode LibPSDHelper::WriteFile(const FilePath& fileName, const Vector<Image*>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const
{
    Logger::Error("[LibPSDHelper::WriteFile] PSD writing is not supported");
    return eErrorCode::ERROR_WRITE_FAIL;
}

eErrorCode LibPSDHelper::WriteFileAsCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const
{
    Logger::Error("[LibPSDHelper::WriteFileAsCubeMap] PSD writing is not supported");
    return eErrorCode::ERROR_WRITE_FAIL;
}

DAVA::ImageInfo LibPSDHelper::GetImageInfo(File* infile) const
{
    ImageInfo info;

    return info;
}
};
