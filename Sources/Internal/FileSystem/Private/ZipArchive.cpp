#include "FileSystem/Private/ZipArchive.h"
#include "FileSystem/FilePath.h"
#include "Logger/Logger.h"
#include "Base/Exception.h"

namespace DAVA
{
ZipArchive::ZipArchive(RefPtr<File>& file_, const FilePath& fileName)
    : zipFile(file_, fileName)
{
    // Get and print information about each file in the archive.
    uint32 count = zipFile.GetNumFiles();

    fileInfos.reserve(count);
    for (uint32 i = 0u; i < count; i++)
    {
        String name;
        uint32 origSize = 0;
        uint32 compressedSize = 0;
        bool isDirectory = false;

        if (!zipFile.GetFileInfo(i, name, origSize, compressedSize, isDirectory))
        {
            DAVA_THROW(DAVA::Exception, "failed! get file info");
        }

        if (!isDirectory)
        {
            ResourceArchive::FileInfo info;
            info.relativeFilePath = name;
            info.compressionType = Compressor::Type::RFC1951;
            info.compressedSize = compressedSize;
            info.originalSize = origSize;
            fileInfos.push_back(info);
        }
    }

    std::stable_sort(begin(fileInfos), end(fileInfos), [](const ResourceArchive::FileInfo& left, const ResourceArchive::FileInfo& right)
                     {
                         return left.relativeFilePath < right.relativeFilePath;
                     });
}

const Vector<ResourceArchive::FileInfo>& ZipArchive::GetFilesInfo() const
{
    return fileInfos;
}

const ResourceArchive::FileInfo* ZipArchive::GetFileInfo(const String& relativeFilePath) const
{
    auto it = std::lower_bound(begin(fileInfos), end(fileInfos), relativeFilePath, [](const ResourceArchive::FileInfo& left, const String& fileNameParam)
                               {
                                   return left.relativeFilePath < fileNameParam;
                               });

    if (it != end(fileInfos) && it->relativeFilePath == relativeFilePath)
    {
        return &(*it);
    }
    return nullptr;
}

bool ZipArchive::HasFile(const String& relativeFilePath) const
{
    return GetFileInfo(relativeFilePath) != nullptr;
}

bool ZipArchive::LoadFile(const String& relativeFilePath, Vector<uint8>& output) const
{
    const ResourceArchive::FileInfo* info = GetFileInfo(relativeFilePath);
    if (info != nullptr)
    {
        output.resize(info->originalSize);

        if (!zipFile.LoadFile(relativeFilePath, output))
        {
            Logger::Error("can't extract file: %s into memory", relativeFilePath.c_str());
            return false;
        }
        return true;
    }
    return false;
}
} // end namespace DAVA
