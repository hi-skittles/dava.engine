#pragma once

#include "Compression/Compressor.h"
#include "Base/Exception.h"

namespace DAVA
{
// catch this type of exception if you need to find situation when user
// changed local resources inside resource archive
class FileCrc32FromPackNotMatch : public Exception
{
public:
    explicit FileCrc32FromPackNotMatch(const String& msg, const char* file, int line)
        : Exception(msg, file, line)
    {
    }
};

class ResourceArchiveImpl;

class FilePath;

class ResourceArchive final
{
public:
    explicit ResourceArchive(const FilePath& filePath);
    ~ResourceArchive();

    struct FileInfo
    {
        FileInfo() = default;
        FileInfo(const char8* relativePath, uint32 originalSize, uint32 originalCrc32, uint32 compressedSize, uint32 compressedCrc32, Compressor::Type compressionType);

        String relativeFilePath;
        uint32 originalSize = 0;
        uint32 originalCrc32 = 0;
        uint32 compressedSize = 0;
        uint32 compressedCrc32 = 0;
        Compressor::Type compressionType = Compressor::Type::None;
    };

    const Vector<FileInfo>& GetFilesInfo() const;
    const FileInfo* GetFileInfo(const String& relativeFilePath) const;
    bool HasFile(const String& relativeFilePath) const;
    bool LoadFile(const String& relativeFilePath, Vector<uint8>& outputFileContent) const;

    bool UnpackToFolder(const FilePath& dir) const;

private:
    std::unique_ptr<ResourceArchiveImpl> impl;
};
} // end namespace DAVA
