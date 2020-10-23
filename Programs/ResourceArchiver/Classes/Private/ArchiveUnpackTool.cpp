#include "ArchiveUnpackTool.h"

#include <Logger/Logger.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/FileList.h>
#include <FileSystem/Private/PackArchive.h>
#include <Job/JobManager.h>
#include <Engine/Engine.h>
#include <Compression/LZ4Compressor.h>
#include <Compression/ZipCompressor.h>

#include "ResultCodes.h"

static int UnpackFile(const DAVA::FilePath& archivePath,
                      const DAVA::PackFormat::PackFile::FilesTableBlock::FilesData::Data& fileInfo,
                      const DAVA::String& relativeFilePath,
                      const bool extractInDvplFormat);

ArchiveUnpackTool::ArchiveUnpackTool()
    : CommandLineTool("unpack")
{
    options.AddArgument("packfile");
    options.AddArgument("directory");

    options.AddOption("-dvpl", DAVA::VariantType(false), "use this flag to extract files in *.dvpl format");
}

bool ArchiveUnpackTool::ConvertOptionsToParamsInternal()
{
    packFilename = options.GetArgument("packfile");
    if (packFilename.IsEmpty())
    {
        DAVA::Logger::Error("packfile param is not specified");
        return false;
    }

    dstDir = options.GetArgument("directory");
    if (dstDir.IsEmpty())
    {
        DAVA::Logger::Error("directory param is not specified");
        return false;
    }
    dstDir.MakeDirectoryPathname();

    if (options.IsOptionExists("-dvpl"))
    {
        extractInDvplFormat = options.GetOption("-dvpl").AsBool();
    }

    return true;
}

int ArchiveUnpackTool::ProcessInternal()
{
    using namespace DAVA;

    FileSystem* fs = FileSystem::Instance();
    FilePath currentDir = fs->GetCurrentWorkingDirectory();
    SCOPE_EXIT
    {
        fs->SetCurrentWorkingDirectory(currentDir);
    };

    if (fs->CreateDirectory(dstDir) == FileSystem::DIRECTORY_CANT_CREATE)
    {
        Logger::Error("Can't create dir '%s'", dstDir.GetAbsolutePathname().c_str());
        return ERROR_CANT_CREATE_DIR;
    }

    if (fs->SetCurrentWorkingDirectory(dstDir) == false)
    {
        Logger::Error("Can't change current dir to '%s'", dstDir.GetAbsolutePathname().c_str());
        return ERROR_CANT_CHANGE_DIR;
    }

    try
    {
        RefPtr<File> f(File::Create(packFilename, File::OPEN | File::READ));
        if (!f)
        {
            return ERROR_CANT_OPEN_ARCHIVE;
        }

        PackArchive packArchive(f, packFilename);

        const auto& packFile = packArchive.GetPackFile();

        JobManager* jobManager = GetEngineContext()->jobManager;
        DVASSERT(jobManager != nullptr);

        std::atomic<int> countExtractedFiles(0);

        const auto& fileInfoBase = packArchive.GetFilesInfo();

        for (size_t i = 0; i < packFile.filesTable.data.files.size(); ++i)
        {
            jobManager->CreateWorkerJob([&, i]()
                                        {
                                            const auto& fileInfoFromArchive = packFile.filesTable.data.files[i];
                                            const auto& fileInfo = fileInfoBase[i];

                                            if (UnpackFile(packFilename, fileInfoFromArchive, fileInfo.relativeFilePath, extractInDvplFormat) == OK)
                                            {
                                                ++countExtractedFiles;
                                            }
                                            else
                                            {
                                                Logger::Error("failed extract file: %s, from archive: %s", fileInfo.relativeFilePath.c_str(), packFilename.GetAbsolutePathname().c_str());
                                            }
                                        });

            if (i > 0 && i % 1000 == 0)
            {
                //HACK wait jobs executed (internaly 1024 max jobs)
                jobManager->WaitWorkerJobs();
            }
        }

        jobManager->WaitWorkerJobs();

        if (countExtractedFiles != packFile.filesTable.data.files.size())
        {
            return ERROR_CANT_EXTRACT_FILE;
        }

        return OK;
    }
    catch (std::exception& ex)
    {
        Logger::Error("Can't open archive %s: %s", packFilename.GetAbsolutePathname().c_str(), ex.what());
        return ERROR_CANT_OPEN_ARCHIVE;
    }
}

static int UnpackFile(const DAVA::FilePath& archivePath,
                      const DAVA::PackFormat::PackFile::FilesTableBlock::FilesData::Data& fileInfo,
                      const DAVA::String& relativeFilePath,
                      const bool extractInDvplFormat)
{
    using namespace DAVA;

    ScopedPtr<File> ifs(File::Create(archivePath.GetAbsolutePathname(), File::OPEN | File::READ));
    if (!ifs)
    {
        return ERROR_CANT_EXTRACT_FILE;
    }

    if (!ifs->Seek(fileInfo.startPosition, File::eFileSeek::SEEK_FROM_START))
    {
        return ERROR_CANT_EXTRACT_FILE;
    }

    Vector<uint8> compressedContent(fileInfo.compressedSize);

    if (fileInfo.compressedSize != ifs->Read(compressedContent.data(), static_cast<uint32>(compressedContent.size())))
    {
        return ERROR_CANT_EXTRACT_FILE;
    }

    FilePath fullPath(relativeFilePath);
    FilePath dirPath = fullPath.GetDirectory();
    FileSystem* const fs = GetEngineContext()->fileSystem;
    if (!fs->Exists(dirPath))
    {
        const FileSystem::eCreateDirectoryResult result = fs->CreateDirectory(dirPath, true);
        if (FileSystem::DIRECTORY_CANT_CREATE == result && !fs->Exists(dirPath)) // in multithreading work we have to dowble check directory exist
        {
            Logger::Error("Can't create unpack path dir %s", dirPath.GetAbsolutePathname().c_str());
            return ERROR_CANT_CREATE_DIR;
        }
    }

    if (extractInDvplFormat)
    {
        PackFormat::LitePack::Footer liteFooter;
        liteFooter.type = fileInfo.type;
        liteFooter.crc32Compressed = fileInfo.compressedCrc32;
        liteFooter.sizeCompressed = fileInfo.compressedSize;
        liteFooter.sizeUncompressed = fileInfo.originalSize;
        liteFooter.packMarkerLite = PackFormat::FILE_MARKER_LITE;
        static_assert(sizeof(liteFooter) == 20, "format not changed");

        fullPath += ".dvpl";

        ScopedPtr<File> file(File::Create(fullPath, File::CREATE | File::WRITE));
        if (!file)
        {
            Logger::Error("Can't create file %s", fullPath.GetAbsolutePathname().c_str());
            return ERROR_CANT_WRITE_FILE;
        }

        const uint32 dataSize = static_cast<uint32>(compressedContent.size());
        const uint32 written = file->Write(compressedContent.data(), dataSize);
        if (written != dataSize)
        {
            Logger::Error("Can't write into %s", fullPath.GetAbsolutePathname().c_str());
            return ERROR_CANT_WRITE_FILE;
        }

        if (sizeof(liteFooter) != file->Write(&liteFooter, sizeof(liteFooter)))
        {
            Logger::Error("Can't write into %s", fullPath.GetAbsolutePathname().c_str());
            return ERROR_CANT_WRITE_FILE;
        }

        return OK;
    }

    Vector<uint8> content;

    switch (fileInfo.type)
    {
    case Compressor::Type::None:
        content = std::move(compressedContent);
        break;
    case Compressor::Type::Lz4:
        content.resize(fileInfo.originalSize);
        if (!LZ4Compressor().Decompress(compressedContent, content))
        {
            return ERROR_CANT_EXTRACT_FILE;
        }
        break;
    case Compressor::Type::Lz4HC:
        content.resize(fileInfo.originalSize);
        if (!LZ4HCCompressor().Decompress(compressedContent, content))
        {
            return ERROR_CANT_EXTRACT_FILE;
        }
        break;
    case Compressor::Type::RFC1951:
        content.resize(fileInfo.originalSize);
        if (!ZipCompressor().Decompress(compressedContent, content))
        {
            return ERROR_CANT_EXTRACT_FILE;
        }
        break;
    default:
        Logger::Error("unknown compression type: %d", fileInfo.type);
        return ERROR_CANT_EXTRACT_FILE;
    }

    if (!fullPath.IsDirectoryPathname())
    {
        ScopedPtr<File> file(File::Create(fullPath, File::CREATE | File::WRITE));
        if (!file)
        {
            Logger::Error("Can't create file %s", fullPath.GetAbsolutePathname().c_str());
            return ERROR_CANT_WRITE_FILE;
        }

        const uint32 dataSize = static_cast<uint32>(content.size());
        const uint32 written = file->Write(content.data(), dataSize);
        if (written != dataSize)
        {
            Logger::Error("Can't write into %s", fullPath.GetAbsolutePathname().c_str());
            return ERROR_CANT_WRITE_FILE;
        }
    }

    return OK;
}
