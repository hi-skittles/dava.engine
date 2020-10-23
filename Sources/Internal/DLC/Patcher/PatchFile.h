#ifndef __DAVAENGINE_TOOLS_PATCH_FILE_H__
#define __DAVAENGINE_TOOLS_PATCH_FILE_H__

#include "FileSystem/FilePath.h"
#include "BSDiff.h"

namespace DAVA
{
class File;

// ======================================================================================
// information about patch
// ======================================================================================
struct PatchInfo
{
    friend class PatchFileReader;
    friend class PatchFileWriter;

    String origPath;
    uint32 origCRC;
    uint32 origSize;

    String newPath;
    uint32 newSize;
    uint32 newCRC;

    PatchInfo();

protected:
    void Reset();
    bool Write(File* file);
    bool Read(File* file);

private:
    bool ReadString(File* file, String&);
    bool WriteString(File* file, const String&);
};

// ======================================================================================
// class for creating/writing patch file
// ======================================================================================
class PatchFileWriter
{
public:
    enum WriterMode
    {
        WRITE, // Create an empty file for output operations
        APPEND // Open file for output at the end of a file. The file will be created if it does not exist.
    };

    PatchFileWriter(const FilePath& path, WriterMode mode, BSType diffType, bool beVerbose = false);
    ~PatchFileWriter();

    void SetLogsFilePath(const FilePath& path);

    // TODO:
    // description
    bool Write(const FilePath& origBase, const FilePath& origPath, const FilePath& newBase, const FilePath& newPath);

protected:
    bool SingleWrite(const FilePath& origBase, const FilePath& origPath, const FilePath& newBase, const FilePath& newPath);
    void EnumerateDir(const FilePath& path, const FilePath& base, List<String>& in);

    DAVA::FilePath patchPath;
    BSType diffType;
    bool verbose;
};

// ======================================================================================
// class for reading/applying patch file
// ======================================================================================
class PatchFileReader
{
public:
    enum PatchError
    {
        ERROR_NO = 0,
        ERROR_MEMORY, // can't allocate memory
        ERROR_CANT_READ, // path file can't be read
        ERROR_CORRUPTED, // path file is corrupted
        ERROR_EMPTY_PATCH, // no data to apply patch
        ERROR_ORIG_READ, // file on origPath can't be opened for reading
        ERROR_ORIG_FILE_CRC, // file on origPath has wrong crc to apply patch
        ERROR_ORIG_BUFFER_CRC, // file readed from origPath has wrong crc to apply patch
        ERROR_NEW_CREATE, // file on newPath can't be opened for writing
        ERROR_NEW_WRITE, // file on newPath can't be written
        ERROR_NEW_CRC, // file on newPath has wrong crc after applied patch
        ERROR_UNKNOWN
    };

    struct PatchingErrorDetails
    {
        struct FileInfo
        {
            FilePath path = "";
            uint32 size = 0;
            uint32 crc = 0;
        };

        FileInfo expected;
        FileInfo actual;
    };

    PatchFileReader(const FilePath& path, bool beVerbose = false, bool enablePermissive = false);
    ~PatchFileReader();

    bool ReadFirst();
    bool ReadLast();
    bool ReadNext();
    bool ReadPrev();

    const PatchInfo* GetCurInfo() const;

    void SetLogsFilePath(const FilePath& path);
    int32 GetFileError() const;
    PatchFileReader::PatchError GetParseError() const;
    PatchFileReader::PatchError GetError() const;
    PatchFileReader::PatchingErrorDetails GetLastErrorDetails() const;

    bool Truncate();
    bool Apply(const FilePath& origBase, const FilePath& origPath, const FilePath& newBase, const FilePath& newPath);

protected:
    bool isPermissiveMode;
    File* patchFile;
    PatchInfo curInfo;
    FilePath logFilePath;
    PatchError lastError;
    PatchError parseError;
    int32 lastFileErrno;
    bool verbose;
    bool eof;
    PatchingErrorDetails lastErrorDetails;

    Vector<int32> patchPositions;
    size_t initialPositionsCount;
    size_t curPatchIndex;
    uint32 curBSDiffPos;

    bool DoRead();
    bool ReadDataBack(void* data, uint32 size);
};

inline void PatchFileReader::SetLogsFilePath(const DAVA::FilePath& path)
{
    logFilePath = path;
}

inline int32 PatchFileReader::GetFileError() const
{
    return lastFileErrno;
}

inline PatchFileReader::PatchError PatchFileReader::GetParseError() const
{
    return parseError;
}

inline PatchFileReader::PatchError PatchFileReader::GetError() const
{
    return lastError;
}

inline PatchFileReader::PatchingErrorDetails PatchFileReader::GetLastErrorDetails() const
{
    return lastErrorDetails;
}
}

#endif // __DAVAENGINE_TOOLS_RESOURCE_PATCHER_H__
