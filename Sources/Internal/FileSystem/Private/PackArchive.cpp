#include "FileSystem/Private/PackArchive.h"
#include "Compression/ZipCompressor.h"
#include "Compression/LZ4Compressor.h"
#include "FileSystem/FileSystem.h"
#include "Utils/CRC32.h"
#include "Logger/Logger.h"
#include "Base/Exception.h"

#include <mutex>

namespace DAVA
{
void PackArchive::ExtractFileTableData(const PackFormat::PackFile::FooterBlock& footerBlock,
                                       const Vector<uint8>& tmpBuffer,
                                       String& fileNames,
                                       PackFormat::PackFile::FilesTableBlock& fileTableBlock)
{
    Vector<uint8>& compressedNamesBuffer = fileTableBlock.names.compressedNames;
    compressedNamesBuffer.resize(footerBlock.info.namesSizeCompressed, '\0');

    uint32 sizeOfFilesData = footerBlock.info.numFiles * sizeof(PackFormat::FileTableEntry);
    const uint8* startOfCompressedNames = &tmpBuffer[sizeOfFilesData];

    fileTableBlock.names.compressedNames.resize(footerBlock.info.namesSizeCompressed);

    std::copy_n(startOfCompressedNames, footerBlock.info.namesSizeCompressed, fileTableBlock.names.compressedNames.data());

    Vector<uint8> originalNamesBuffer;
    originalNamesBuffer.resize(footerBlock.info.namesSizeOriginal);
    if (!LZ4HCCompressor().Decompress(compressedNamesBuffer, originalNamesBuffer))
    {
        DAVA_THROW(DAVA::Exception, "can't uncompress file names");
    }

    fileNames = String(begin(originalNamesBuffer), end(originalNamesBuffer));

    Vector<PackFormat::FileTableEntry>& fileTable = fileTableBlock.data.files;
    fileTable.resize(footerBlock.info.numFiles);

    const PackFormat::FileTableEntry* startFilesData = reinterpret_cast<const PackFormat::FileTableEntry*>(tmpBuffer.data());

    std::copy_n(startFilesData, footerBlock.info.numFiles, fileTable.data());
}

void PackArchive::FillFilesInfo(const PackFormat::PackFile& packFile,
                                const String& fileNames,
                                UnorderedMap<String, const PackFormat::FileTableEntry*>& mapFileData,
                                Vector<ResourceArchive::FileInfo>& filesInfo)
{
    filesInfo.reserve(packFile.footer.info.numFiles);

    size_t numFiles =
    std::count_if(begin(fileNames), end(fileNames), [](const char& ch)
                  {
                      return '\0' == ch;
                  });

    const Vector<PackFormat::FileTableEntry>& fileTable = packFile.filesTable.data.files;

    if (numFiles != fileTable.size())
    {
        DAVA_THROW(DAVA::Exception, "number of file names not match with table");
    }

    // now fill support structures for fast search by filename
    size_t fileNameIndex{ 0 };

    std::for_each(begin(fileTable), end(fileTable), [&](const PackFormat::FileTableEntry& fileEntry)
                  {
                      const char* fileNameLoc = &fileNames[fileNameIndex];
                      mapFileData.emplace(fileNameLoc, &fileEntry);

                      ResourceArchive::FileInfo info;

                      info.relativeFilePath = fileNameLoc;
                      info.originalSize = fileEntry.originalSize;
                      info.originalCrc32 = fileEntry.originalCrc32;
                      info.compressedSize = fileEntry.compressedSize;
                      info.compressedCrc32 = fileEntry.compressedCrc32;
                      info.compressionType = fileEntry.type;

                      filesInfo.push_back(info);

                      fileNameIndex = fileNames.find('\0', fileNameIndex + 1);
                      ++fileNameIndex;
                  });
}

PackArchive::PackArchive(RefPtr<File>& file_, const FilePath& archiveName_)
    : archiveName(archiveName_)
    , file(file_)
{
    using namespace PackFormat;

    String fileName = archiveName.GetAbsolutePathname();

    if (!file)
    {
        DAVA_THROW(DAVA::Exception, "can't Open file: " + fileName);
    }

    uint64 size = file->GetSize();
    if (size < sizeof(packFile.footer))
    {
        DAVA_THROW(DAVA::Exception, "file size less then pack footer: " + fileName);
    }

    if (!file->Seek(size - sizeof(packFile.footer), File::SEEK_FROM_START))
    {
        DAVA_THROW(DAVA::Exception, "can't seek to footer in file: " + fileName);
    }

    auto& footerBlock = packFile.footer;
    uint32 numBytesRead = file->Read(&footerBlock, sizeof(footerBlock));
    if (numBytesRead != sizeof(footerBlock))
    {
        DAVA_THROW(DAVA::Exception, "can't read footer from packfile: " + fileName);
    }

    uint32 crc32footer = CRC32::ForBuffer(reinterpret_cast<char*>(&packFile.footer.info), sizeof(packFile.footer.info));
    if (crc32footer != packFile.footer.infoCrc32)
    {
        DAVA_THROW(DAVA::Exception, "crc32 not match in footer for: " + fileName);
    }

    if (footerBlock.info.packArchiveMarker != FILE_MARKER)
    {
        DAVA_THROW(DAVA::Exception, "incorrect marker in pack file: " + fileName);
    }

    String fileNames;
    if (footerBlock.info.numFiles > 0)
    {
        uint64 startFilesTableBlock = size - (sizeof(packFile.footer) + packFile.footer.info.filesTableSize);

        Vector<uint8> tmpBuffer;
        tmpBuffer.resize(packFile.footer.info.filesTableSize);

        if (!file->Seek(startFilesTableBlock, File::SEEK_FROM_START))
        {
            DAVA_THROW(DAVA::Exception, "can't seek to filesTable block in file: " + fileName);
        }

        numBytesRead = file->Read(tmpBuffer.data(), packFile.footer.info.filesTableSize);
        if (numBytesRead != packFile.footer.info.filesTableSize)
        {
            DAVA_THROW(DAVA::Exception, "can't read filesTable block from file: " + fileName);
        }

        uint32 crc32filesTable = CRC32::ForBuffer(tmpBuffer.data(), packFile.footer.info.filesTableSize);
        if (crc32filesTable != packFile.footer.info.filesTableCrc32)
        {
            DAVA_THROW(DAVA::Exception, "crc32 not match in filesTable in file: " + fileName);
        }

        ExtractFileTableData(footerBlock, tmpBuffer, fileNames, packFile.filesTable);

        FillFilesInfo(packFile, fileNames, mapFileData, filesInfo);
    }

    if (footerBlock.metaDataSize > 0)
    {
        // parse metadata block
        uint64_t startMetaBlock = size - (sizeof(packFile.footer) + packFile.footer.info.filesTableSize + footerBlock.metaDataSize);
        std::vector<uint8> metaBlock(footerBlock.metaDataSize);
        if (!file->Seek(startMetaBlock, File::SEEK_FROM_START))
        {
            DAVA_THROW(Exception, "can't seek meta");
        }
        uint32 metaSize = static_cast<uint32>(metaBlock.size());
        if (file->Read(&metaBlock[0], metaSize) != metaSize)
        {
            DAVA_THROW(Exception, "can't read meta");
        }
        packMeta.reset(new PackMetaData(&metaBlock[0], metaBlock.size(), fileNames));
    }
}

const Vector<ResourceArchive::FileInfo>& PackArchive::GetFilesInfo() const
{
    return filesInfo;
}

const ResourceArchive::FileInfo* PackArchive::GetFileInfo(const String& relativeFilePath) const
{
    auto it = mapFileData.find(relativeFilePath);

    if (it != mapFileData.end())
    {
        // find out index of FileInfo*
        const PackFormat::FileTableEntry* currentFile = it->second;
        const PackFormat::FileTableEntry* start = packFile.filesTable.data.files.data();
        ptrdiff_t index = std::distance(start, currentFile);
        return &filesInfo.at(static_cast<uint32>(index));
    }
    return nullptr;
}

bool PackArchive::HasFile(const String& relativeFilePath) const
{
    auto iterator = mapFileData.find(relativeFilePath);
    return iterator != mapFileData.end();
}

bool PackArchive::LoadFile(const String& relativeFilePath, Vector<uint8>& output) const
{
    using namespace PackFormat;

    if (!HasFile(relativeFilePath))
    {
        return false;
    }

    const FileTableEntry& fileEntry = *mapFileData.find(relativeFilePath)->second;
    output.resize(fileEntry.originalSize);

    if (!file)
    {
        DAVA_THROW(DAVA::Exception, "can't open: " + relativeFilePath + " from pack: " + archiveName.GetStringValue());
    }

    bool isOk = file->Seek(fileEntry.startPosition, File::SEEK_FROM_START);
    if (!isOk)
    {
        Logger::Error("can't load file: %s course: can't find start file position in pack file", relativeFilePath.c_str());
        return false;
    }

    switch (fileEntry.type)
    {
    case Compressor::Type::None:
    {
        uint32 readOk = file->Read(output.data(), fileEntry.originalSize);
        if (readOk != fileEntry.originalSize)
        {
            Logger::Error("can't load file: %s course: can't read uncompressed content", relativeFilePath.c_str());
            return false;
        }
    }
    break;
    case Compressor::Type::Lz4:
    case Compressor::Type::Lz4HC:
    {
        Vector<uint8> packedBuf(fileEntry.compressedSize);

        uint32 readOk = file->Read(packedBuf.data(), fileEntry.compressedSize);
        if (readOk != fileEntry.compressedSize)
        {
            Logger::Error("can't load file: %s course: can't read compressed content", relativeFilePath.c_str());
            return false;
        }

        if (!LZ4Compressor().Decompress(packedBuf, output))
        {
            Logger::Error("can't load file: %s  course: decompress error", relativeFilePath.c_str());
            return false;
        }
    }
    break;
    case Compressor::Type::RFC1951:
    {
        Vector<uint8> packedBuf(fileEntry.compressedSize);

        uint32 readOk = file->Read(packedBuf.data(), fileEntry.compressedSize);
        if (readOk != fileEntry.compressedSize)
        {
            Logger::Error("can't load file: %s course: can't read compressed content", relativeFilePath.c_str());
            return false;
        }

        if (!ZipCompressor().Decompress(packedBuf, output))
        {
            Logger::Error("can't load file: %s  course: decompress error", relativeFilePath.c_str());
            return false;
        }
    }
    break;
    } // end switch

    // check crc32 for file content
    if (fileEntry.originalCrc32 != 0 && fileEntry.originalCrc32 != CRC32::ForBuffer(output.data(), output.size()))
    {
        String msg = "original crc32 not match for: " + relativeFilePath + " during decompress from pack: " + archiveName.GetStringValue();
        throw FileCrc32FromPackNotMatch(msg, __FILE__, __LINE__);
    }

    return true;
}

uint32 PackArchive::GetFileIndex(const String& releativeFilePath) const
{
    uint32 result = std::numeric_limits<uint32>::max();

    const ResourceArchive::FileInfo* info = GetFileInfo(releativeFilePath);
    if (info != nullptr)
    {
        ptrdiff_t index = std::distance(&filesInfo[0], info);
        result = static_cast<uint32>(index);
    }
    return result;
}

bool PackArchive::HasMeta() const
{
    return packMeta.get() != nullptr;
}

const PackMetaData& PackArchive::GetMeta() const
{
    return *packMeta;
}

const PackFormat::PackFile& PackArchive::GetPackFile() const
{
    return packFile;
}

} // end namespace DAVA
