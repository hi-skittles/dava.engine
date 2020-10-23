#pragma once

#include "FileSystem/Private/ResourceArchivePrivate.h"
#include "Compression/ZipCompressor.h"

namespace DAVA
{
class ZipArchive final : public ResourceArchiveImpl
{
public:
    explicit ZipArchive(RefPtr<File>& file_, const FilePath& archiveName);

    const Vector<ResourceArchive::FileInfo>& GetFilesInfo() const override;
    const ResourceArchive::FileInfo* GetFileInfo(const String& relativeFilePath) const override;
    bool HasFile(const String& relativeFilePath) const override;
    bool LoadFile(const String& relativeFilePath, Vector<uint8>& output) const override;

private:
    ZipFile zipFile;
    Vector<ResourceArchive::FileInfo> fileInfos;
};
} // end namespace DAVA
