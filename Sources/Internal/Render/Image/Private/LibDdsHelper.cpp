#include "Render/Image/LibDdsHelper.h"
#include "Render/Image/Image.h"
#include "Render/Image/Private/DDSHandlers.h"
#include "Render/Texture.h"

#include "Base/Exception.h"
#include "FileSystem/FileSystem.h"
#include "Logger/Logger.h"
#include "Utils/CRC32.h"

namespace DAVA
{
LibDdsHelper::LibDdsHelper()
    : ImageFormatInterface(IMAGE_FORMAT_DDS, "DDS", { ".dds" },
                           { FORMAT_ATC_RGB, // supported pixel formats
                             FORMAT_ATC_RGBA_EXPLICIT_ALPHA,
                             FORMAT_ATC_RGBA_INTERPOLATED_ALPHA,
                             FORMAT_DXT1,
                             FORMAT_REMOVED_DXT_1N,
                             FORMAT_DXT1A,
                             FORMAT_DXT3,
                             FORMAT_DXT5,
                             FORMAT_DXT5NM,
                             FORMAT_RGBA8888 })
{
}

bool LibDdsHelper::CanProcessFileInternal(File* infile) const
{
    std::unique_ptr<DDSReader> reader = DDSReader::CreateReader(infile);
    return (reader.get() != nullptr);
}

eErrorCode LibDdsHelper::ReadFile(File* infile, Vector<Image*>& imageSet, const ImageSystem::LoadingParams& loadingParams) const
{
    DVASSERT(infile);

    std::unique_ptr<DDSReader> reader = DDSReader::CreateReader(infile);
    if (reader && reader->GetImages(imageSet, loadingParams))
    {
        return eErrorCode::SUCCESS;
    }

    return eErrorCode::ERROR_FILE_FORMAT_INCORRECT;
}

eErrorCode LibDdsHelper::WriteFileInternal(const FilePath& outFileName, const Vector<Vector<Image*>>& imageSet, PixelFormat dstFormat, ImageQuality quality) const
{
    ScopedPtr<File> file(File::Create(outFileName, File::CREATE | File::WRITE));
    std::unique_ptr<DDSWriter> ddsWriter = DDSWriter::CreateWriter(file);

    if (ddsWriter && ddsWriter->Write(imageSet, dstFormat))
    {
        return DAVA::eErrorCode::SUCCESS;
    }

    return DAVA::eErrorCode::ERROR_WRITE_FAIL;
}

eErrorCode LibDdsHelper::WriteFile(const FilePath& outFileName, const Vector<Image*>& imageSet, PixelFormat dstFormat, ImageQuality quality) const
{
    return WriteFileInternal(outFileName, { imageSet }, dstFormat, quality);
}

eErrorCode LibDdsHelper::WriteFileAsCubeMap(const FilePath& outFileName, const Vector<Vector<Image*>>& imageSet, PixelFormat dstFormat, ImageQuality quality) const
{
    if (imageSet.size() != Texture::CUBE_FACE_COUNT)
    {
        Logger::Error("[LibDdsHelper::WriteFileAsCubeMap] Wrong input image set in attempt to write into %s", outFileName.GetStringValue().c_str());
        return eErrorCode::ERROR_WRITE_FAIL;
    }

    return WriteFileInternal(outFileName, imageSet, dstFormat, quality);
}

ImageInfo LibDdsHelper::GetImageInfo(File* infile) const
{
    std::unique_ptr<DDSReader> reader(DDSReader::CreateReader(infile));
    return (reader ? reader->GetImageInfo() : ImageInfo());
}

bool LibDdsHelper::AddCRCIntoMetaData(const FilePath& filePathname)
{
    ScopedPtr<File> ddsFile(File::Create(filePathname, File::OPEN | File::READ | File::WRITE));
    if (!ddsFile)
    {
        Logger::Error("[LibDdsHelper::AddCRCIntoMetaData] cannot open file %s", filePathname.GetStringValue().c_str());
        return false;
    }

    std::unique_ptr<DDSReader> ddsReader(DDSReader::CreateReader(ddsFile));
    if (ddsReader)
    {
        return ddsReader->AddCRC();
    }
    else
    {
        Logger::Error("[LibDdsHelper::AddCRCIntoMetaData] is not a DDS file %s", filePathname.GetStringValue().c_str());
        return false;
    }
}

uint32 LibDdsHelper::GetCRCFromMetaData(const FilePath& filePathname)
{
    ScopedPtr<File> ddsFile(File::Create(filePathname, File::READ | File::OPEN));
    if (ddsFile)
    {
        std::unique_ptr<DDSReader> reader(DDSReader::CreateReader(ddsFile));
        if (reader)
        {
            uint32 crc = 0;
            if (reader->GetCRC(crc))
            {
                return crc;
            }
        }
        else
        {
            Logger::Error("[LibDdsHelper::GetCRCFromFile] %s is not a DDS file", filePathname.GetStringValue().c_str());
        }
    }

    return 0;
}
}
