#include "Compression/ZipCompressor.h"
#include "FileSystem/File.h"
#include "Logger/Logger.h"
#include "Base/Exception.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

#define MINIZ_NO_STDIO

// Disable warning C4334 on VS2015
#if _MSC_VER >= 1900

    #pragma warning(push)
    #pragma warning(disable : 4334)
    #include <miniz/miniz.c>
    #pragma warning(pop)

#else

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

    #include <miniz/miniz.c>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace DAVA
{
bool ZipCompressor::Compress(const Vector<uint8>& in, Vector<uint8>& out) const
{
    uLong destMaxLength = compressBound(static_cast<uint32>(in.size()));
    if (out.size() < destMaxLength)
    {
        out.resize(destMaxLength);
    }
    uLong resultLength = destMaxLength;
    int32 result = compress(out.data(), &resultLength, in.data(), static_cast<uLong>(in.size()));
    if (result != Z_OK)
    {
        Logger::Error("can't compress rfc1951 buffer");
        return false;
    }
    out.resize(resultLength);
    return true;
}

bool ZipCompressor::Decompress(const Vector<uint8>& in, Vector<uint8>& out) const
{
    if (in.size() > static_cast<uint32>(std::numeric_limits<uLong>::max()))
    {
        Logger::Error("too big input buffer for uncompress rfc1951");
        return false;
    }
    uLong uncompressedSize = static_cast<uLong>(out.size());
    int32 decompressResult = uncompress(out.data(), &uncompressedSize, in.data(), static_cast<uLong>(in.size()));
    if (decompressResult != Z_OK)
    {
        Logger::Error("can't uncompress rfc1951 buffer");
        return false;
    }
    out.resize(uncompressedSize);
    return true;
}

class ZipPrivateData
{
public:
    FilePath archiveFileName;
    mz_zip_archive archive;
    RefPtr<File> file{ nullptr };
};

static size_t file_read_func(void* pOpaque, mz_uint64 file_ofs, void* pBuf, size_t n)
{
    File* file = static_cast<File*>(pOpaque);
    if (!file)
    {
        DVASSERT(false && "pOpaque not point to DAVA::File*");
        Logger::Error("nullptr zip archive File object");
        return 0;
    }

    DVASSERT(file_ofs < std::numeric_limits<uint32>::max());
    if (!file->Seek(static_cast<uint32>(file_ofs), File::SEEK_FROM_START))
    {
        Logger::Error("can't set seek pos to %d in zip archive file", static_cast<uint32>(file_ofs));
        return 0;
    }

    uint32 result = file->Read(pBuf, static_cast<uint32>(n));
    if (result != n)
    {
        DVASSERT(false && "can't read bytes from zip archive");
        Logger::Error("can't read bytes from zip archive");
    }

    return static_cast<size_t>(result);
}

static size_t file_write_func(void* pOpaque, mz_uint64 file_ofs, const void* pBuf, size_t n)
{
    File* file = static_cast<File*>(pOpaque);
    if (!file)
    {
        DVASSERT(false && "pOpaque not point to DAVA::File*");
        Logger::Error("nullptr zip archive File object");
        return 0;
    }

    DVASSERT(file_ofs < std::numeric_limits<uint32>::max());
    if (!file->Seek(static_cast<uint32>(file_ofs), File::SEEK_FROM_START))
    {
        Logger::Error("can't set seek pos to %d in zip archive file", static_cast<uint32>(file_ofs));
        return 0;
    }

    uint32 result = file->Write(pBuf, static_cast<uint32>(n));
    if (result != n)
    {
        DVASSERT(false && "can't write bytes from zip archive");
        Logger::Error("can't write bytes from zip archive");
    }

    return static_cast<size_t>(result);
}

ZipFile::ZipFile(RefPtr<File>& file_, const FilePath& fileName)
{
    zipData.reset(new ZipPrivateData());

    Memset(&zipData->archive, 0, sizeof(zipData->archive));

    zipData->archiveFileName = fileName;
    zipData->file = file_;

    if (!zipData->file)
    {
        DAVA_THROW(DAVA::Exception, "can't open archive file: " + fileName.GetStringValue());
    }

    uint64 fileSize = zipData->file->GetSize();
    zipData->archive.m_pIO_opaque = zipData->file.Get();
    zipData->archive.m_pRead = &file_read_func;
    zipData->archive.m_pWrite = &file_write_func;
    zipData->archive.m_archive_size = fileSize;
}

ZipFile::~ZipFile()
{
    Finalize();
}

uint32 ZipFile::GetNumFiles() const
{
    StartRead();

    return mz_zip_reader_get_num_files(&zipData->archive);
}

DAVA::int32 ZipFile::GetFileIndex(const String& relativeFilePath) const
{
    StartRead();

    return mz_zip_reader_locate_file(&zipData->archive, relativeFilePath.c_str(), nullptr, 0);
}

bool ZipFile::Exists(const String& relativeFilePath) const
{
    return GetFileIndex(relativeFilePath) >= 0;
}

bool ZipFile::GetFileInfo(uint32 fileIndex, String& fileName, uint32& fileOriginalSize, uint32& fileCompressedSize, bool& isDirectory) const
{
    StartRead();

    mz_zip_archive_file_stat fileStat;
    if (!mz_zip_reader_file_stat(&zipData->archive, fileIndex, &fileStat))
    {
        Logger::Error("can't get file status from zip archive: %s", fileName.c_str());
        return false;
    }

    fileName = fileStat.m_filename;
    fileOriginalSize = static_cast<uint32>(fileStat.m_uncomp_size);
    fileCompressedSize = static_cast<uint32>(fileStat.m_comp_size);
    isDirectory = (mz_zip_reader_is_file_a_directory(&zipData->archive, fileIndex) != 0);

    return true;
}

bool ZipFile::LoadFile(const String& fileName, Vector<uint8>& fileContent) const
{
    StartRead();

    int32 fileIndex = mz_zip_reader_locate_file(&zipData->archive, fileName.c_str(), nullptr, 0);
    if (fileIndex < 0)
    {
        Logger::Error("file: %s not found in archive: %s!", fileName.c_str(), fileName.c_str());
        return false;
    }

    mz_zip_archive_file_stat fileStat;
    if (!mz_zip_reader_file_stat(&zipData->archive, fileIndex, &fileStat))
    {
        Logger::Error("mz_zip_reader_file_stat() failed!");
        return false;
    }

    if (fileContent.size() != fileStat.m_uncomp_size)
    {
        fileContent.resize(static_cast<size_t>(fileStat.m_uncomp_size));
    }

    mz_bool result = mz_zip_reader_extract_file_to_mem(&zipData->archive, fileName.c_str(), fileContent.data(), fileContent.size(), 0);
    if (result == MZ_FALSE)
    {
        Logger::Error("can't extract file: %s into memory", fileName.c_str());
        return false;
    }
    return true;
}

bool ZipFile::SaveFile(const String& relativeFilePath, Vector<uint8>& fileContent) const
{
    StartWrite();

    if (MZ_FALSE == mz_zip_writer_add_mem(&zipData->archive, relativeFilePath.c_str(), fileContent.data(), fileContent.size(), Z_DEFAULT_COMPRESSION))
    {
        return false;
    }

    return true;
}

void ZipFile::StartWrite() const
{
    if (MZ_ZIP_MODE_WRITING == zipData->archive.m_zip_mode)
    {
        return;
    }

    Finalize();

    if (mz_zip_writer_init(&zipData->archive, 0) == MZ_FALSE)
    {
        DAVA_THROW(DAVA::Exception, "can't init writer zip from file: " + zipData->archiveFileName.GetStringValue());
    }
}

void ZipFile::StartRead() const
{
    if (MZ_ZIP_MODE_READING == zipData->archive.m_zip_mode)
    {
        return;
    }

    Finalize();

    if (MZ_FALSE == mz_zip_reader_init(&zipData->archive, zipData->file->GetSize(), 0))
    {
        DAVA_THROW(DAVA::Exception, "can't init reader zip from file: " + zipData->archiveFileName.GetStringValue());
    }
}

void ZipFile::Finalize() const
{
    mz_zip_mode& mode = zipData->archive.m_zip_mode;

    if (mode == MZ_ZIP_MODE_READING)
    {
        if (MZ_FALSE == mz_zip_reader_end(&zipData->archive))
        {
            Logger::Error("zip file: can't end reader");
        }
    }

    if (mode == MZ_ZIP_MODE_WRITING)
    {
        if (MZ_FALSE == mz_zip_writer_finalize_archive(&zipData->archive))
        {
            Logger::Error("zip file: can't end writer");
        }
    }

    if (mode == MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED)
    {
        if (MZ_FALSE == mz_zip_writer_end(&zipData->archive))
        {
            Logger::Error("zip file: can't finalize archive");
        }
    }
}

} // end namespace DAVA
