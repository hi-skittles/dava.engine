#include "FileSystem/FileAPIHelper.h"
#include "FileSystem/Private/CheckIOError.h"
#include "Utils/UTF8Utils.h"
#include "Logger/Logger.h"

#include <sys/stat.h>

namespace DAVA
{
namespace FileAPI
{
#ifdef __DAVAENGINE_WINDOWS__
using Stat = struct ::_stat;
const auto FileStat = _wstat;
#else
using Stat = struct ::stat;
const auto FileStat = stat;
#endif

FILE* OpenFile(const String& fileName, const String& mode)
{
#ifdef __DAVAENGINE_WINDOWS__
    WideString f = UTF8Utils::EncodeToWideString(fileName);
    WideString m = UTF8Utils::EncodeToWideString(mode);
    return _wfopen(f.c_str(), m.c_str());
#else
    return fopen(fileName.c_str(), mode.c_str());
#endif
}

int32 Close(FILE* f)
{
    int32 result = EOF;
    if (f != nullptr)
    {
        result = fclose(f);
        if (result != 0)
        {
            Logger::Error("error during close file stream");
        }
    }
    return result;
}

int32 RemoveFile(const String& fileName)
{
#ifdef __DAVAENGINE_WINDOWS__
    WideString f = UTF8Utils::EncodeToWideString(fileName);
    return _wremove(f.c_str());
#else
    return remove(fileName.c_str());
#endif
}

int32 RenameFile(const String& oldFileName, const String& newFileName)
{
#ifdef __DAVAENGINE_DEBUG__
    if (DebugFS::GenErrorOnMoveFailed())
    {
        return 1;
    }
#endif

#ifdef __DAVAENGINE_WINDOWS__
    WideString old = UTF8Utils::EncodeToWideString(oldFileName);
    WideString new_ = UTF8Utils::EncodeToWideString(newFileName);
    return _wrename(old.c_str(), new_.c_str());
#else
    return rename(oldFileName.c_str(), newFileName.c_str());
#endif
}

static void LogError(int32 errnoCode, const String& fileName, const char* functionName)
{
    switch (errnoCode)
    {
    case ENOENT:
        // file not found
        break;
    case EINVAL:
        Logger::Error("Invalid parameter to stat: %s", fileName.c_str());
        break;
    case EACCES:
        // common case for android: /mnt/knox/Android/
        // do not spam to logger
        break;
    default:
        // Should never be reached.
        Logger::Error("Unexpected error in func: %s: errno: %s for path: %s",
                      functionName,
                      strerror(errnoCode),
                      fileName.c_str());
    }
}

bool IsRegularFile(const String& fileName)
{
    Stat fileStat;

#ifdef __DAVAENGINE_WINDOWS__
    WideString p = UTF8Utils::EncodeToWideString(fileName);
    int32 result = FileStat(p.c_str(), &fileStat);
#else
    int32 result = FileStat(fileName.c_str(), &fileStat);
#endif
    if (result == 0)
    {
        return (0 != (fileStat.st_mode & S_IFREG));
    }

    LogError(errno, fileName, __FUNCTION__);
    return false;
}

bool IsDirectory(const String& dirName)
{
#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR) /* directory */
#define CLEAR_S_ISDIR_TMP_VAR 1
#endif
    Stat fileStat;

#ifdef __DAVAENGINE_WINDOWS__
    WideString p = UTF8Utils::EncodeToWideString(dirName);
    int32 result = FileStat(p.c_str(), &fileStat);
#else
    int32 result = FileStat(dirName.c_str(), &fileStat);
#endif
    if (result == 0)
    {
        return (0 != (S_ISDIR(fileStat.st_mode)));
    }

    LogError(errno, dirName, __FUNCTION__);
#ifdef CLEAR_S_ISDIR_TMP_VAR
#undef S_ISDIR
#undef CLEAR_S_ISDIR_TMP_VAR
#endif
    return false;
}

uint64 GetFileSize(const String& fileName)
{
    Stat fileStat;

#ifdef __DAVAENGINE_WINDOWS__
    WideString p = UTF8Utils::EncodeToWideString(fileName);
    int32 result = FileStat(p.c_str(), &fileStat);
#else
    int32 result = FileStat(fileName.c_str(), &fileStat);
#endif
    if (result == 0)
    {
        return static_cast<uint64>(fileStat.st_size);
    }

    LogError(errno, fileName, __FUNCTION__);

    return std::numeric_limits<uint64>::max();
}

} // end namespace FileAPI
} // end namespace DAVA
