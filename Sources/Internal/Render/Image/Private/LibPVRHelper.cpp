#include "Render/Image/LibPVRHelper.h"
#include "Render/Image/Image.h"
#include "Render/Image/ImageConvert.h"
#include "Render/Image/ImageSystem.h"
#include "Render/Image/Private/PVRFormatHelper.h"
#include "Render/PixelFormatDescriptor.h"

#include "Render/Texture.h"
#include "Logger/Logger.h"
#include "FileSystem/FileSystem.h"
#include "Utils/Utils.h"
#include "Utils/CRC32.h"

namespace DAVA
{
LibPVRHelper::LibPVRHelper()
    : ImageFormatInterface(ImageFormat::IMAGE_FORMAT_PVR, "PVR", { ".pvr" },
                           {
                           FORMAT_RGBA8888,
                           FORMAT_RGBA5551,
                           FORMAT_RGBA4444,
                           FORMAT_RGB888,
                           FORMAT_RGB565,
                           FORMAT_A8,
                           FORMAT_A16,
                           FORMAT_PVR2,
                           FORMAT_PVR4,
                           FORMAT_ETC1,
                           })
{
}

bool LibPVRHelper::CanProcessFileInternal(File* file) const
{
    std::unique_ptr<PVRFile> pvrFile = PVRFormatHelper::ReadFile(file, false, false);
    if (pvrFile)
    {
        return (PVRTEX3_IDENT == pvrFile->header.u32Version);
    }
    return false;
}

eErrorCode LibPVRHelper::Load(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const
{
    if (PVRFormatHelper::LoadImages(infile, imageSet, loadingParams))
    {
        DVASSERT(imageSet.empty() == false);
        return eErrorCode::SUCCESS;
    }

    return eErrorCode::ERROR_READ_FAIL;
}

eErrorCode LibPVRHelper::ReadFile(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const
{
    eErrorCode loadResult = Load(infile, imageSet, loadingParams);
    if (eErrorCode::SUCCESS != loadResult || imageSet.empty())
    {
        return loadResult;
    }

    return eErrorCode::SUCCESS;
}

eErrorCode LibPVRHelper::WriteFile(const FilePath& fileName, const Vector<Image*>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const
{
    if (imageSet.empty())
    {
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    if (imageSet[0]->format != compressionFormat)
    {
        return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
    }

    return Save(fileName, imageSet);
}

eErrorCode LibPVRHelper::WriteFileAsCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet, PixelFormat compressionFormat, ImageQuality quality) const
{
    if (imageSet.empty())
    {
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    for (const Vector<Image*>& imgSet : imageSet)
    {
        if (imgSet.empty())
        {
            return eErrorCode::ERROR_WRITE_FAIL;
        }

        if (imgSet[0]->format != compressionFormat)
        {
            return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
        }
    }

    return SaveCubeMap(fileName, imageSet);
}

eErrorCode LibPVRHelper::Save(const FilePath& fileName, const Vector<Image*>& imageSet) const
{
    if (imageSet.empty())
    {
        Logger::Error("Can't save empty images vector");
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    std::unique_ptr<PVRFile> pvrFile = PVRFormatHelper::CreateHeader(imageSet);
    if (pvrFile)
    {
        ScopedPtr<File> file(File::Create(fileName, File::CREATE | File::WRITE));
        if (file && PVRFormatHelper::WriteFile(file, *pvrFile))
        {
            for (const Image* image : imageSet)
            {
                uint32 written = 0;

                //TODO: this code should be re-worked with texture refactoring
                if (image->format == PixelFormat::FORMAT_RGBA4444 || image->format == PixelFormat::FORMAT_RGBA5551)
                {
                    ScopedPtr<Image> savedImage(Image::Create(image->width, image->height, image->format));
                    DVASSERT(savedImage->dataSize == image->dataSize);

                    if (image->format == PixelFormat::FORMAT_RGBA4444)
                    {
                        ConvertDirect<uint16, uint16, ConvertRGBA4444toABGR4444> convert;
                        convert(image->data, image->width, image->height, image->width * sizeof(uint16), savedImage->data);
                    }
                    else if (image->format == PixelFormat::FORMAT_RGBA5551)
                    {
                        ConvertDirect<uint16, uint16, ConvertRGBA5551toABGR1555> convert;
                        convert(image->data, image->width, image->height, image->width * sizeof(uint16), savedImage->data);
                    }

                    written = file->Write(savedImage->data, savedImage->dataSize);
                }
                else
                {
                    written = file->Write(image->data, image->dataSize);
                }
                //END of TODO

                if (written != image->dataSize)
                {
                    return eErrorCode::ERROR_WRITE_FAIL;
                }
            }
            return eErrorCode::SUCCESS;
        }
    }

    return eErrorCode::ERROR_WRITE_FAIL;
}

eErrorCode LibPVRHelper::SaveCubeMap(const FilePath& fileName, const Vector<Vector<Image*>>& imageSet) const
{
    uint32 faceCount = static_cast<uint32>(imageSet.size());
    if (faceCount != Texture::CUBE_FACE_COUNT)
    {
        Logger::Error("Can't save cube images vector due wrong face count: %u of %u", faceCount, Texture::CUBE_FACE_COUNT);
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    uint32 mipmapsCount = static_cast<uint32>(imageSet[0].size());
    for (const Vector<Image*>& faceImageSet : imageSet)
    {
        uint32 faceMipmpsCount = static_cast<uint32>(faceImageSet.size());
        if (faceMipmpsCount == 0 || faceMipmpsCount != mipmapsCount)
        {
            Logger::Error("Can't save cube images vector due wrong mip count: %u of %u", faceMipmpsCount, mipmapsCount);
            return eErrorCode::ERROR_WRITE_FAIL;
        }
    }

    std::unique_ptr<PVRFile> pvrFile = PVRFormatHelper::CreateCubeHeader(imageSet);
    if (pvrFile)
    {
        ScopedPtr<File> file(File::Create(fileName, File::CREATE | File::WRITE));
        if (file && PVRFormatHelper::WriteFile(file, *pvrFile))
        {
            for (uint32 mip = 0; mip < pvrFile->header.u32MIPMapCount; ++mip)
            {
                for (const Vector<Image*>& faceImageSet : imageSet)
                {
                    uint32 written = 0;

                    Image* image = faceImageSet[mip];

                    //TODO: this code should be re-worked with texture refactoring
                    if (image->format == PixelFormat::FORMAT_RGBA4444 || image->format == PixelFormat::FORMAT_RGBA5551)
                    {
                        ScopedPtr<Image> savedImage(Image::Create(image->width, image->height, image->format));
                        DVASSERT(savedImage->dataSize == image->dataSize);

                        if (image->format == PixelFormat::FORMAT_RGBA4444)
                        {
                            ConvertDirect<uint16, uint16, ConvertRGBA4444toABGR4444> convert;
                            convert(image->data, image->width, image->height, image->width * sizeof(uint16), savedImage->data);
                        }
                        else if (image->format == PixelFormat::FORMAT_RGBA5551)
                        {
                            ConvertDirect<uint16, uint16, ConvertRGBA5551toABGR1555> convert;
                            convert(image->data, image->width, image->height, image->width * sizeof(uint16), savedImage->data);
                        }

                        written = file->Write(savedImage->data, savedImage->dataSize);
                    }
                    else
                    {
                        written = file->Write(image->data, image->dataSize);
                    }
                    //END of TODO

                    if (written != image->dataSize)
                    {
                        return eErrorCode::ERROR_WRITE_FAIL;
                    }
                }
            }

            return eErrorCode::SUCCESS;
        }
    }

    return eErrorCode::ERROR_WRITE_FAIL;
}

DAVA::ImageInfo LibPVRHelper::GetImageInfo(File* infile) const
{
    ImageInfo info;

    std::unique_ptr<PVRFile> pvrFile = PVRFormatHelper::ReadFile(infile, false, false);
    if (pvrFile)
    {
        info.width = pvrFile->header.u32Width;
        info.height = pvrFile->header.u32Height;
        info.format = PVRFormatHelper::GetPixelFormat(pvrFile->header);
        info.dataSize = static_cast<uint32>(infile->GetSize() - (PVRFile::HEADER_SIZE + pvrFile->header.u32MetaDataSize));
        info.mipmapsCount = pvrFile->header.u32MIPMapCount;
        info.faceCount = pvrFile->header.u32NumFaces;
    }

    return info;
}

bool LibPVRHelper::AddCRCIntoMetaData(const FilePath& filePathname)
{
    //read file
    std::unique_ptr<PVRFile> pvrFile = PVRFormatHelper::ReadFile(filePathname, true, true);
    if (!pvrFile)
    {
        return false;
    }

    //read crc
    uint32 presentCRC = 0;
    bool crcIsPresent = PVRFormatHelper::GetCRCFromMetaData(*pvrFile, &presentCRC);
    if (crcIsPresent)
    {
        return false;
    }

    uint32 calculatedCRC = CRC32::ForFile(filePathname);
    PVRFormatHelper::AddCRCToMetaData(*pvrFile, calculatedCRC);

    return PVRFormatHelper::WriteFile(filePathname, *pvrFile);
}

uint32 LibPVRHelper::GetCRCFromMetaData(const FilePath& filePathname)
{
    std::unique_ptr<PVRFile> pvrFile = PVRFormatHelper::ReadFile(filePathname, true, false);
    if (pvrFile)
    {
        uint32 presentCRC = 0;
        bool crcIsPresent = PVRFormatHelper::GetCRCFromMetaData(*pvrFile, &presentCRC);
        if (crcIsPresent)
        {
            return presentCRC;
        }
    }

    return 0;
}
}