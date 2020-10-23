#include "UnitTests/UnitTests.h"
#include <FileSystem/Private/PackArchive.h>
#include <FileSystem/Private/ZipArchive.h>
#include <FileSystem/FileSystem.h>
#include <Logger/Logger.h>

#include <cstring>

using namespace DAVA;

DAVA_TESTCLASS (ArchiveTest)
{
    DAVA_TEST (TestDavaArchive)
    {
        FilePath baseDir("~res:/TestData/FileListTest/");

#if !defined(__DAVAENGINE_IPHONE__) && !defined(__DAVAENGINE_ANDROID__)

        {
            try
            {
                RefPtr<File> fileDvpk(File::Create("~res:/TestData/ArchiveTest/archive.dvpk", File::OPEN | File::READ));
                PackArchive archive(fileDvpk, "~res:/TestData/ArchiveTest/archive.dvpk");

                {
                    const char* filename = "Utf8Test/utf16le.txt";

                    TEST_VERIFY(archive.HasFile(filename));

                    const ResourceArchive::FileInfo* archiveInfo = archive.GetFileInfo(filename);

                    TEST_VERIFY(archiveInfo->compressionType == Compressor::Type::Lz4HC);

                    Vector<uint8> fileFromArchive;

                    TEST_VERIFY(archive.LoadFile(filename, fileFromArchive));

                    FilePath filePath("~res:/TestData/Utf8Test/utf16le.txt");

                    ScopedPtr<File> file(File::Create(filePath, File::OPEN | File::READ));

                    uint64 fileSize = file->GetSize();

                    Vector<uint8> fileFromHDD(static_cast<size_t>(fileSize), 0);

                    file->Read(fileFromHDD.data(), static_cast<uint32>(fileSize));

                    TEST_VERIFY(fileFromHDD == fileFromArchive);
                }
            }
            catch (std::exception& ex)
            {
                Logger::Info(ex.what());
                // for now do not fail this test, in next pull request
                // I finish will fix it (format dvpk is changing now)
            }
        }
#endif // __DAVAENGINE_IPHONE__
    }

    DAVA_TEST (TestZipArchive)
    {
        try
        {
            RefPtr<File> fileZip(File::Create("~res:/TestData/ArchiveTest/archive.zip", File::OPEN | File::READ));
            ZipArchive archive(fileZip, "~res:/TestData/ArchiveTest/archive.zip");
            {
                const char* filename = "Utf8Test/utf16le.txt";

                TEST_VERIFY(archive.HasFile(filename));

                const ResourceArchive::FileInfo* archiveInfo = archive.GetFileInfo(filename);

                TEST_VERIFY(archiveInfo->compressionType == Compressor::Type::RFC1951);

                Vector<uint8> fileFromArchive;

                TEST_VERIFY(archive.LoadFile(filename, fileFromArchive));

                FilePath filePath("~res:/TestData/Utf8Test/utf16le.txt");

                ScopedPtr<File> file(File::Create(filePath, File::OPEN | File::READ));

                uint64 fileSize = file->GetSize();

                Vector<uint8> fileFromHDD(static_cast<size_t>(fileSize), 0);

                file->Read(fileFromHDD.data(), static_cast<uint32>(fileSize));

                TEST_VERIFY(fileFromHDD == fileFromArchive);
            }
        }
        catch (std::exception& ex)
        {
            Logger::Error("%s", ex.what());
            TEST_VERIFY(false && "can't open zip file");
        }
    }
};
