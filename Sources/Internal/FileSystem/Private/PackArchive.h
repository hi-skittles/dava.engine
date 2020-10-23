#pragma once

#include "FileSystem/Private/ResourceArchivePrivate.h"
#include "FileSystem/Private/PackFormatSpec.h"
#include "FileSystem/Private/PackMetaData.h"
#include "FileSystem/File.h"

namespace DAVA
{
class PackArchive final : public ResourceArchiveImpl
{
public:
    PackArchive(RefPtr<File>& file_, const FilePath& archiveName);

    const Vector<ResourceArchive::FileInfo>& GetFilesInfo() const override;
    const ResourceArchive::FileInfo* GetFileInfo(const String& relativeFilePath) const override;
    bool HasFile(const String& relativeFilePath) const override;
    bool LoadFile(const String& relativeFilePath, Vector<uint8>& output) const override;

    /**
		return index of struct with file info, usefull for meta data
		on error return numeric_limits<uint32>::max
	*/
    uint32 GetFileIndex(const String& releativeFilePath) const;

    bool HasMeta() const;
    /**
		check if meta exist with HasMeta();
	*/
    const PackMetaData& GetMeta() const;

    const PackFormat::PackFile& GetPackFile() const;

    static void ExtractFileTableData(const PackFormat::PackFile::FooterBlock& footerBlock,
                                     const Vector<uint8>& tmpBuffer,
                                     String& fileNames,
                                     PackFormat::PackFile::FilesTableBlock& fileTableBlock);

    static void FillFilesInfo(const PackFormat::PackFile& packFile,
                              const String& fileNames,
                              UnorderedMap<String,
                                           const PackFormat::FileTableEntry*>& mapFileData,
                              Vector<ResourceArchive::FileInfo>& filesInfo);

private:
    const FilePath archiveName;
    mutable RefPtr<File> file;
    PackFormat::PackFile packFile;
    std::unique_ptr<PackMetaData> packMeta;
    UnorderedMap<String, const PackFormat::FileTableEntry*> mapFileData;
    Vector<ResourceArchive::FileInfo> filesInfo;
};

} // end namespace DAVA
