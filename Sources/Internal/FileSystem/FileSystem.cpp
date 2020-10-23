#include <sys/types.h>
#include <sys/stat.h>

#include "Base/Platform.h"
#include "Base/Exception.h"
#include "Debug/Backtrace.h"

#include "FileSystem/FileAPIHelper.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/FileSystemDelegate.h"
#include "FileSystem/FileList.h"
#include "FileSystem/YamlNode.h"
#include "Debug/DVAssert.h"
#include "Utils/Utils.h"
#include "Logger/Logger.h"
#include "FileSystem/ResourceArchive.h"
#include "Concurrency/LockGuard.h"

#include "Engine/Private/EngineBackend.h"

#if defined(__DAVAENGINE_MACOS__)
#include <copyfile.h>
#include <libproc.h>
#include <libgen.h>
#include <unistd.h>
#include <CoreServices/CoreServices.h>
#elif defined(__DAVAENGINE_IPHONE__)
#include <copyfile.h>
#include <libgen.h>
#include <sys/sysctl.h>
#include <unistd.h>
#elif defined(__DAVAENGINE_WINDOWS__)
#include <direct.h>
#include <io.h>
#include <Shlobj.h>
#include <tchar.h>
#include <process.h>
#if defined(__DAVAENGINE_WIN_UAP__)
#include "Platform/DeviceInfo.h"
#endif
#elif defined(__DAVAENGINE_ANDROID__)
#include "Engine/Private/Android/AssetsManagerAndroid.h"
#include "Engine/Private/Android/AndroidBridge.h"
#include <unistd.h>
#elif defined(__DAVAENGINE_LINUX__)
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#endif //PLATFORMS

namespace DAVA
{
static Set<String> androidAssetsFiles;

FileSystem::FileSystem()
{
}

FileSystem::~FileSystem()
{
    // All locked files should be explicitly unlocked before closing the app.
    DVASSERT(lockedFileHandles.empty());
}

FileSystem::eCreateDirectoryResult FileSystem::CreateDirectory(const FilePath& filePath, bool isRecursive)
{
    DVASSERT(filePath.GetType() != FilePath::PATH_IN_RESOURCES);

    if (!isRecursive)
    {
        return CreateExactDirectory(filePath);
    }

    String path = filePath.GetAbsolutePathname();

    Vector<String> tokens;
    Split(path, "/", tokens);

    String dir = "";

#if defined(__DAVAENGINE_WINDOWS__)
    if (0 < tokens.size() && 0 < tokens[0].length())
    {
        String::size_type pos = path.find(tokens[0]);
        if (String::npos != pos)
        {
            tokens[0] = path.substr(0, pos) + tokens[0];
        }
    }
#else //#if defined (__DAVAENGINE_WINDOWS__)
    String::size_type find = path.find(":");
    if (find == String::npos)
    {
        dir = "/";
    }
#endif //#if defined (__DAVAENGINE_WINDOWS__)

    for (size_t k = 0; k < tokens.size(); ++k)
    {
        dir += tokens[k] + "/";

        eCreateDirectoryResult ret = CreateExactDirectory(dir);
        if (k == tokens.size() - 1)
        {
            return ret;
        }
    }
    return DIRECTORY_CANT_CREATE;
}

FileSystem::eCreateDirectoryResult FileSystem::CreateExactDirectory(const FilePath& filePath)
{
    DVASSERT(filePath.GetType() != FilePath::PATH_IN_RESOURCES);

    if (IsDirectory(filePath))
        return DIRECTORY_EXISTS;

#ifdef __DAVAENGINE_WINDOWS__
    WideString path = UTF8Utils::EncodeToWideString(filePath.GetAbsolutePathname());
    BOOL res = ::CreateDirectoryW(path.c_str(), 0);
    return (res == 0) ? DIRECTORY_CANT_CREATE : DIRECTORY_CREATED;
#elif defined(__DAVAENGINE_POSIX__)
    int res = mkdir(filePath.GetAbsolutePathname().c_str(), 0777);
    return (res == 0) ? (DIRECTORY_CREATED) : (DIRECTORY_CANT_CREATE);
#endif //PLATFORMS
}

bool FileSystem::CopyFile(const FilePath& existingFile, const FilePath& newFile, bool overwriteExisting /* = false */)
{
    DVASSERT(newFile.GetType() != FilePath::PATH_IN_RESOURCES);

#ifdef __DAVAENGINE_WINDOWS__
    WideString existingFilePath = UTF8Utils::EncodeToWideString(existingFile.GetAbsolutePathname());
    WideString newFilePath = UTF8Utils::EncodeToWideString(newFile.GetAbsolutePathname());
#endif

#ifdef __DAVAENGINE_WIN32__

    BOOL ret = ::CopyFileW(existingFilePath.c_str(), newFilePath.c_str(), !overwriteExisting);
    return ret != 0;

#elif defined(__DAVAENGINE_WIN_UAP__)

    COPYFILE2_EXTENDED_PARAMETERS params =
    {
      /* dwSize */ sizeof(COPYFILE2_EXTENDED_PARAMETERS),
      /* dwCopyFlags */ overwriteExisting ? DWORD(0) : COPY_FILE_FAIL_IF_EXISTS
    };
    return ::CopyFile2(existingFilePath.c_str(), newFilePath.c_str(), &params) == S_OK;

#elif defined(__DAVAENGINE_ANDROID__) || defined(__DAVAENGINE_LINUX__)
    // TODO: try sendfile for linux

    bool copied = false;

    File* srcFile = File::Create(existingFile, File::OPEN | File::READ);
    File* dstFile = File::Create(newFile, File::WRITE | File::CREATE);

    Logger::Debug("copy file from %s(%p) to %s(%p)", existingFile.GetStringValue().c_str(),
                  srcFile, newFile.GetStringValue().c_str(), dstFile);

    if (srcFile && dstFile)
    {
        uint32 fileSize = srcFile->GetSize();
        uint8* data = new uint8[fileSize];
        if (data)
        {
            uint32 read = srcFile->Read(data, fileSize);
            if (read == fileSize)
            {
                uint32 written = dstFile->Write(data, fileSize);
                if (written == fileSize)
                {
                    copied = true;
                }
                else
                {
                    Logger::Error("[FileSystem::CopyFile] can't write to file %s", newFile.GetAbsolutePathname().c_str());
                }
            }
            else
            {
                Logger::Error("[FileSystem::CopyFile] can't read file %s", existingFile.GetAbsolutePathname().c_str());
            }

            SafeDeleteArray(data);
        }
        else
        {
            Logger::Error("[FileSystem::CopyFile] can't allocate memory of %d Bytes", fileSize);
        }
    }

    SafeRelease(dstFile);
    SafeRelease(srcFile);

    return copied;

#else //iphone & macos
    int ret = copyfile(existingFile.GetAbsolutePathname().c_str(), newFile.GetAbsolutePathname().c_str(), NULL, overwriteExisting ? COPYFILE_ALL : COPYFILE_ALL | COPYFILE_EXCL);
    return ret == 0;
#endif //PLATFORMS
}

bool FileSystem::MoveFile(const FilePath& existingFile, const FilePath& newFile, bool overwriteExisting /* = false*/)
{
    DVASSERT(newFile.GetType() != FilePath::PATH_IN_RESOURCES);

    String toFile = newFile.GetAbsolutePathname();
    String fromFile = existingFile.GetAbsolutePathname();

    if (overwriteExisting)
    {
        FileAPI::RemoveFile(toFile);
    }
    else
    {
        if (IsFile(newFile))
        {
            return false;
        }
    }
    int result = FileAPI::RenameFile(fromFile, toFile);
    if (0 != result && EXDEV == errno)
    {
        result = CopyFile(existingFile, newFile);
        if (result)
        {
            result = DeleteFile(existingFile);
            if (result)
            {
                result = 0;
            }
        }
    }
    bool error = (0 != result);
    if (error)
    {
        const char* errorReason = strerror(errno);
        Logger::Error("rename failed (\"%s\" -> \"%s\") with error: %s",
                      existingFile.GetStringValue().c_str(),
                      newFile.GetStringValue().c_str(),
                      errorReason);
    }
    return !error;
}

bool FileSystem::CopyDirectoryFiles(const FilePath& sourceDirectory, const FilePath& destinationDirectory, bool overwriteExisting /* = false */)
{
    DVASSERT(sourceDirectory.IsDirectoryPathname() && destinationDirectory.IsDirectoryPathname());

    bool ret = true;

    ScopedPtr<FileList> fileList(new FileList(sourceDirectory));
    int32 count = fileList->GetCount();
    String fileOnly;
    String pathOnly;
    for (int32 i = 0; i < count; ++i)
    {
        if (!fileList->IsDirectory(i) && !fileList->IsNavigationDirectory(i))
        {
            const FilePath destinationPath = destinationDirectory + fileList->GetFilename(i);
            if (!CopyFile(fileList->GetPathname(i), destinationPath, overwriteExisting))
            {
                ret = false;
            }
        }
    }

    return ret;
}

bool FileSystem::DeleteFile(const FilePath& filePath)
{
    DVASSERT(filePath.GetType() != FilePath::PATH_IN_RESOURCES);

    // function unlink return 0 on success, -1 on error
    String fileName = filePath.GetAbsolutePathname();

    int res = FileAPI::RemoveFile(fileName);
    if (res == 0)
    {
        return true;
    }

    if (errno == ENOENT) // no such file
    {
        return false;
    }
    Logger::Error("can't delete file %s cause: %s", filePath.GetStringValue().c_str(), strerror(errno));
    return false;
}

bool FileSystem::DeleteDirectory(const FilePath& path, bool isRecursive)
{
    DVASSERT(path.GetType() != FilePath::PATH_IN_RESOURCES);
    DVASSERT(path.IsDirectoryPathname());

    if (!IsDirectory(path))
    {
        return false;
    }

    ScopedPtr<FileList> fileList(new FileList(path));
    for (uint32 i = 0; i < fileList->GetCount(); ++i)
    {
        if (fileList->IsDirectory(i))
        {
            if (!fileList->IsNavigationDirectory(i))
            {
                if (isRecursive)
                {
                    //					Logger::FrameworkDebug("- try to delete directory: %s / %s", fileList->GetPathname(i).c_str(), fileList->GetFilename(i).c_str());
                    bool success = DeleteDirectory(fileList->GetPathname(i), isRecursive);
                    //					Logger::FrameworkDebug("- delete directory: %s / %s- %d", fileList->GetPathname(i).c_str(), fileList->GetFilename(i).c_str(), success ? (1): (0));
                    if (!success)
                        return false;
                }
            }
        }
        else
        {
            bool success = DeleteFile(fileList->GetPathname(i));
            //			Logger::FrameworkDebug("- delete file: %s / %s- %d", fileList->GetPathname(i).c_str(), fileList->GetFilename(i).c_str(), success ? (1): (0));
            if (!success)
                return false;
        }
    }

#ifdef __DAVAENGINE_WINDOWS__
    WideString sysPath = UTF8Utils::EncodeToWideString(path.GetAbsolutePathname());
    int32 chmodres = _wchmod(sysPath.c_str(), _S_IWRITE); // change read-only file mode
    int32 res = _wrmdir(sysPath.c_str());
    return (res == 0);
#elif defined(__DAVAENGINE_POSIX__)
    int32 res = rmdir(path.GetAbsolutePathname().c_str());
    return (res == 0);
#endif //PLATFORMS
}

uint32 FileSystem::DeleteDirectoryFiles(const FilePath& path, bool isRecursive)
{
    DVASSERT(path.GetType() != FilePath::PATH_IN_RESOURCES);
    DVASSERT(path.IsDirectoryPathname());

    uint32 fileCount = 0;

    ScopedPtr<FileList> fileList(new FileList(path));
    for (uint32 i = 0; i < fileList->GetCount(); ++i)
    {
        if (fileList->IsDirectory(i))
        {
            if (!fileList->IsNavigationDirectory(i))
            {
                if (isRecursive)
                {
                    fileCount += DeleteDirectoryFiles(fileList->GetPathname(i), isRecursive);
                }
            }
        }
        else
        {
            bool success = DeleteFile(fileList->GetPathname(i));
            if (success)
                fileCount++;
        }
    }

    return fileCount;
}

Vector<FilePath> FileSystem::EnumerateFilesInDirectory(const FilePath& path, bool isRecursive)
{
    ScopedPtr<FileList> fileList(new FileList(path));
    Vector<FilePath> result;

    for (uint32 i = 0; i < fileList->GetCount(); ++i)
    {
        if (fileList->IsNavigationDirectory(i))
        {
            continue;
        }
        else if (fileList->IsDirectory(i))
        {
            if (isRecursive)
            {
                Vector<FilePath> subDirList = EnumerateFilesInDirectory(fileList->GetPathname(i));
                std::move(subDirList.begin(), subDirList.end(), std::back_inserter(result));
            }
        }
        else
        {
            result.push_back(fileList->GetPathname(i));
        }
    }

    return result;
}

File* FileSystem::CreateFileForFrameworkPath(const FilePath& frameworkPath, uint32 attributes)
{
    return File::Create(frameworkPath, attributes);
}

FilePath FileSystem::GetTempDirectoryPath() const
{
#ifdef __DAVAENGINE_WIN_UAP__
    auto folder = Windows::Storage::ApplicationData::Current->TemporaryFolder;
    const wchar_t* ptr = folder->Path->Data();
    return FilePath(ptr);
#else
    static const char* envNames[] = { "TMPDIR", "TMP", "TEMP", "TEMPDIR" };
    for (const char* envName : envNames)
    {
        const char* tmp = std::getenv(envName);
        if (tmp != nullptr)
        {
#if defined(__DAVAENGINE_MACOS__)
            // On macos TEMP path by default contain a symlink as a part of path.
            // This code try to resolve it.
            CFStringRef path = CFStringCreateWithFileSystemRepresentation(0, tmp);
            if (path != nullptr)
            {
                SCOPE_EXIT
                {
                    CFRelease(path);
                };

                CFURLRef url = CFURLCreateWithFileSystemPath(0, path, kCFURLPOSIXPathStyle, TRUE);
                if (url != nullptr)
                {
                    SCOPE_EXIT
                    {
                        CFRelease(url);
                    };

                    CFDataRef bookmarkData = CFURLCreateBookmarkData(0, url, kCFURLBookmarkCreationSuitableForBookmarkFile, NULL, NULL, NULL);
                    if (bookmarkData != nullptr)
                    {
                        SCOPE_EXIT
                        {
                            CFRelease(bookmarkData);
                        };

                        CFURLBookmarkResolutionOptions options = kCFBookmarkResolutionWithoutUIMask | kCFBookmarkResolutionWithoutMountingMask;
                        CFURLRef resolvedUrl = CFURLCreateByResolvingBookmarkData(0, bookmarkData, options, NULL, NULL, NULL, NULL);
                        if (resolvedUrl != nullptr)
                        {
                            CFStringRef cfstr(CFURLCopyFileSystemPath(resolvedUrl, kCFURLPOSIXPathStyle));
                            SCOPE_EXIT
                            {
                                CFRelease(resolvedUrl);
                                CFRelease(cfstr);
                            };

                            CFIndex bufLen = CFStringGetMaximumSizeForEncoding(CFStringGetLength(cfstr), kCFStringEncodingUTF8) + 1;

                            Vector<char> buffer;
                            buffer.resize(bufLen, 0);
                            CFStringGetCString(cfstr, buffer.data(), bufLen, kCFStringEncodingUTF8);

                            return FilePath(buffer.data());
                        }
                    }
                }
            }
#endif
            return FilePath(tmp);
        }
    }
    return FilePath();
#endif
}

FilePath FileSystem::GetCurrentWorkingDirectory()
{
    FilePath currentWorkingDirectory;
#if defined(__DAVAENGINE_WINDOWS__)

    Array<wchar_t, MAX_PATH> tempDir;
    ::GetCurrentDirectoryW(MAX_PATH, tempDir.data());
    currentWorkingDirectory = FilePath(UTF8Utils::EncodeToUTF8(tempDir.data()));

#elif defined(__DAVAENGINE_POSIX__)

    Array<char, PATH_MAX> tempDir;
    getcwd(tempDir.data(), PATH_MAX);
    currentWorkingDirectory = FilePath(tempDir.data());

#endif //PLATFORMS

    return currentWorkingDirectory.MakeDirectoryPathname();
}

FilePath FileSystem::GetCurrentExecutableDirectory()
{
    FilePath currentExecuteDirectory;

#if defined(__DAVAENGINE_WINDOWS__)
    Array<wchar_t, MAX_PATH> tempDir;
    ::GetModuleFileNameW(nullptr, tempDir.data(), MAX_PATH);
    String data = UTF8Utils::EncodeToUTF8(tempDir.data());
    FilePath path(data);
    currentExecuteDirectory = path.GetDirectory();
#elif defined(__DAVAENGINE_MACOS__)
    Array<char, PATH_MAX> tempDir;
    proc_pidpath(getpid(), tempDir.data(), PATH_MAX);
    currentExecuteDirectory = FilePath(dirname(tempDir.data()));
#else
    // dava.engine's internals can invoke GetCurrentExecutableDirectory before Engine instance is created
    const String& str = Private::EngineBackend::Instance()->GetCommandLine().at(0);
    currentExecuteDirectory = FilePath(str).GetDirectory();
#endif //PLATFORMS

    return currentExecuteDirectory.MakeDirectoryPathname();
}

FilePath FileSystem::GetPluginDirectory()
{
    FilePath currentExecuteDirectory = GetCurrentExecutableDirectory();


#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)
    FilePath pluginDirectory = currentExecuteDirectory + "../PlugIns/";

#else
    FilePath pluginDirectory = currentExecuteDirectory + "PlugIns/";

#endif //PLATFORMS

    return pluginDirectory;
}

bool FileSystem::SetCurrentWorkingDirectory(const FilePath& newWorkingDirectory)
{
    DVASSERT(newWorkingDirectory.IsDirectoryPathname());

#if defined(__DAVAENGINE_WINDOWS__)
    WideString path = UTF8Utils::EncodeToWideString(newWorkingDirectory.GetAbsolutePathname());
    BOOL res = ::SetCurrentDirectoryW(path.c_str());
    return (res != 0);
#elif defined(__DAVAENGINE_POSIX__)
    return (chdir(newWorkingDirectory.GetAbsolutePathname().c_str()) == 0);
#elif //PLATFORMS
#error "Unknown platform"
#endif
}

bool FileSystem::IsFile(const FilePath& pathToCheck) const
{
    // ~res:/ or c:/... or ~doc:/
    String nativePath = pathToCheck.GetAbsolutePathname();

    if (fsDelegate != nullptr && fsDelegate->IsFileExists(nativePath) == false)
    { // hooked check: can we continue work with file?
        return false;
    }

    if (FileAPI::IsRegularFile(nativePath))
    {
        return true;
    }

    nativePath += extDvpl;

    if (FileAPI::IsRegularFile(nativePath))
    {
        return true;
    }

#ifdef __DAVAENGINE_ANDROID__
    // ~res:/ or Data/... or tips.yaml
    auto assets = AssetsManagerAndroid::Instance();
    const String& path = pathToCheck.GetAbsolutePathname();
    if (assets->HasFile(path))
    {
        return true;
    }
#endif

    return false;
}

bool FileSystem::IsDirectory(const FilePath& pathToCheck) const
{
    String pathToCheckStr = pathToCheck.GetAbsolutePathname();
    if (fsDelegate != nullptr && fsDelegate->IsDirectoryExists(pathToCheckStr) == false)
    { // hooked check: can we continue work with directory?
        return false;
    }

#if defined(__DAVAENGINE_WIN32__)
    WideString path = UTF8Utils::EncodeToWideString(pathToCheckStr);
    DWORD stats = GetFileAttributesW(path.c_str());
    return (stats != -1) && (0 != (stats & FILE_ATTRIBUTE_DIRECTORY));
#else

    if (FileAPI::IsDirectory(pathToCheckStr))
    {
        return true;
    }

#if defined(__DAVAENGINE_ANDROID__)
    // on android we need test directory in assets inside APK
    if (FilePath::PATH_IN_RESOURCES == pathToCheck.GetType())
    {
        FilePath dirPath(pathToCheck);
        if (dirPath.IsDirectoryPathname())
        {
            dirPath.MakeDirectoryPathname();
        }

        String strDir = dirPath.GetStringValue();
        if (strDir.find("~res:/") == 0 && strDir.size() > 6)
        {
            strDir = strDir.substr(6);
        }

        auto assets = AssetsManagerAndroid::Instance();
        if (assets->HasDirectory(strDir))
        {
            return true;
        }
    }
#endif // __DAVAENGINE_ANDROID__
#endif // __DAVAENGINE_WIN32__
    return false;
}

bool FileSystem::IsHidden(const FilePath& pathToCheck) const
{
#if defined(__DAVAENGINE_WINDOWS__)
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    BOOL areAttributesGot = GetFileAttributesExW(UTF8Utils::EncodeToWideString(pathToCheck.GetStringValue()).c_str(), GetFileExInfoStandard, &fileInfo);
    return (areAttributesGot == TRUE && (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);
#else
    String name = pathToCheck.IsDirectoryPathname() ? pathToCheck.GetLastDirectoryName() : pathToCheck.GetFilename();
    return (!name.empty() && name.front() == '.');
#endif
}

#if defined(__DAVAENGINE_WINDOWS__)
HANDLE CreateFileWin(const String& path, bool shareRead = false)
{
    int share = shareRead ? FILE_SHARE_READ : 0;
    WideString pathWide = UTF8Utils::EncodeToWideString(FilePath(path).GetAbsolutePathname());

#if defined(__DAVAENGINE_WIN32__)

    HANDLE hFile = CreateFileW(pathWide.c_str(), GENERIC_READ, share, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

#elif defined(__DAVAENGINE_WIN_UAP__)

    CREATEFILE2_EXTENDED_PARAMETERS params = { sizeof(CREATEFILE2_EXTENDED_PARAMETERS) };
    params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    params.dwFileFlags = 0;
    params.dwSecurityQosFlags = SECURITY_ANONYMOUS;
    params.lpSecurityAttributes = NULL;
    params.hTemplateFile = NULL;

    HANDLE hFile = CreateFile2(pathWide.c_str(), GENERIC_READ, share, OPEN_ALWAYS, &params);

#endif

    return hFile;
}
#endif

bool FileSystem::LockFile(const FilePath& filePath, bool isLock)
{
    if (!IsFile(filePath))
    {
        return false;
    }

    if (IsFileLocked(filePath) == isLock)
    {
        return true;
    }

    String path = filePath.GetAbsolutePathname();

#if defined(__DAVAENGINE_WINDOWS__)
    if (isLock)
    {
        HANDLE hFile = CreateFileWin(path, true);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            lockedFileHandles[path] = hFile;
            return true;
        }
    }
    else
    {
        Map<String, void*>::iterator lockedFileIter = lockedFileHandles.find(path);
        if (lockedFileIter != lockedFileHandles.end())
        {
            CloseHandle((HANDLE)lockedFileIter->second);
            lockedFileHandles.erase(lockedFileIter);
            return true;
        }
    }

    return false;

#elif defined(__DAVAENGINE_MACOS__)

    if (isLock)
    {
        if (chflags(path.c_str(), UF_IMMUTABLE) == 0)
        {
            lockedFileHandles[path] = NULL; // handle is not needed in case of MacOS.
            return true;
        }
    }
    else
    {
        struct stat s;
        if (stat(path.c_str(), &s) == 0)
        {
            Map<String, void*>::iterator lockedFileIter = lockedFileHandles.find(path);
            if (lockedFileIter != lockedFileHandles.end())
            {
                lockedFileHandles.erase(lockedFileIter);
            }

            s.st_flags &= ~UF_IMMUTABLE;
            return (chflags(path.c_str(), s.st_flags) == 0);
        }
    }

    return false;
#else
    // Not implemented for all other platforms yet.
    DVASSERT(false);
    return false;
#endif
}

bool FileSystem::IsFileLocked(const FilePath& filePath) const
{
    String path = filePath.GetAbsolutePathname();

#if defined(__DAVAENGINE_WINDOWS__)

    HANDLE hFile = CreateFileWin(path);
    DWORD createFileError = GetLastError();

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    CloseHandle(hFile);
    return (createFileError == ERROR_SHARING_VIOLATION);

#elif defined(__DAVAENGINE_MACOS__)

    struct stat s;
    if (stat(path.c_str(), &s) == 0)
    {
        return (0 != (s.st_flags & UF_IMMUTABLE));
    }

    return false;

#else
    // Not implemented for all other platforms yet.
    return false;
#endif
}

const FilePath& FileSystem::GetCurrentDocumentsDirectory()
{
    return currentDocDirectory;
}

void FileSystem::SetCurrentDocumentsDirectory(const FilePath& newDocDirectory)
{
    currentDocDirectory = newDocDirectory;
}

void FileSystem::SetDefaultDocumentsDirectory()
{
    SetCurrentDocumentsDirectory(GetUserDocumentsPath() + "DAVAProject/");
}

#if defined(__DAVAENGINE_WINDOWS__)
const FilePath FileSystem::GetUserDocumentsPath()
{
#if defined(__DAVAENGINE_WIN32__)

    wchar_t szPath[MAX_PATH + 1];
    SHGetFolderPathW(nullptr, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szPath);
    size_t n = wcslen(szPath);
    szPath[n] = L'\\';
    szPath[n + 1] = 0;

    String p = UTF8Utils::EncodeToUTF8(szPath);

    return FilePath(p).MakeDirectoryPathname();

#elif defined(__DAVAENGINE_WIN_UAP__)

    //take local folder as user documents folder
    using ::Windows::Storage::ApplicationData;

    WideString roamingFolder = ApplicationData::Current->LocalFolder->Path->Data();
    String folder = UTF8Utils::EncodeToUTF8(roamingFolder);
    return FilePath(folder).MakeDirectoryPathname();

#endif
}

const FilePath FileSystem::GetPublicDocumentsPath()
{
#if defined(__DAVAENGINE_WIN32__)

    wchar_t szPath[MAX_PATH + 1] = {};
    SHGetFolderPathW(nullptr, CSIDL_COMMON_DOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, szPath);
    size_t n = wcslen(szPath);
    szPath[n] = L'\\';
    szPath[n + 1] = 0;

    String p = UTF8Utils::EncodeToUTF8(szPath);

    return FilePath(p).MakeDirectoryPathname();

#elif defined(__DAVAENGINE_WIN_UAP__)

    //take the first removable storage as public documents folder
    List<DeviceInfo::StorageInfo> storageList = DeviceInfo::GetStoragesList();
    for (const auto& x : storageList)
    {
        if (x.type == DeviceInfo::STORAGE_TYPE_PRIMARY_EXTERNAL ||
            x.type == DeviceInfo::STORAGE_TYPE_SECONDARY_EXTERNAL)
        {
            return x.path;
        }
    }
    // FIXME on desktop only one internal path available better then nothing
    return GetUserDocumentsPath();

#endif
}
#elif defined(__DAVAENGINE_ANDROID__)
const FilePath FileSystem::GetUserDocumentsPath()
{
    return FilePath(Private::AndroidBridge::GetInternalDocumentsDir());
}

const FilePath FileSystem::GetPublicDocumentsPath()
{
    return FilePath(Private::AndroidBridge::GetExternalDocumentsDir());
}
#elif defined(__DAVAENGINE_LINUX__)
const FilePath FileSystem::GetUserDocumentsPath()
{
    // TODO: linux

    // Return HOME directory
    struct passwd pwd
    {
    };
    struct passwd* result = nullptr;

    size_t bufsize = static_cast<size_t>(sysconf(_SC_GETPW_R_SIZE_MAX));
    if (bufsize == size_t(-1))
    {
        // Like in sample in man for getpwuid_r: https://linux.die.net/man/3/getpwuid_r
        bufsize = 16384;
    }

    Vector<char> buf(bufsize);
    int r = getpwuid_r(getuid(), &pwd, buf.data(), bufsize, &result);
    if (r == 0)
    {
        return FilePath(pwd.pw_dir).MakeDirectoryPathname();
    }
    return FilePath();
}

const FilePath FileSystem::GetPublicDocumentsPath()
{
    // TODO: linux
    return GetUserDocumentsPath();
}
#endif

String FileSystem::ReadFileContents(const FilePath& pathname)
{
    String fileContents;
    ScopedPtr<File> fp(File::Create(pathname, File::OPEN | File::READ));
    if (!fp)
    {
        Logger::Error("Failed to open file: %s", pathname.GetAbsolutePathname().c_str());
    }
    else
    {
        uint64 fileSize = fp->GetSize();

        fileContents.resize(static_cast<size_t>(fileSize));

        uint32 dataRead = fp->Read(&fileContents[0], static_cast<uint32>(fileSize));

        if (dataRead != fileSize)
        {
            Logger::Error("Failed to read data from file: %s", pathname.GetAbsolutePathname().c_str());
            fileContents.clear();
        }
    }
    return fileContents;
}

uint8* FileSystem::ReadFileContents(const FilePath& pathname, uint64& fileSize)
{
    File* fp = File::Create(pathname, File::OPEN | File::READ);
    if (!fp)
    {
        Logger::Error("Failed to open file: %s", pathname.GetAbsolutePathname().c_str());
        return 0;
    }
    fileSize = fp->GetSize();
    uint8* bytes = new uint8[static_cast<size_t>(fileSize)];
    uint32 dataRead = fp->Read(bytes, static_cast<uint32>(fileSize));

    if (dataRead != fileSize)
    {
        Logger::Error("Failed to read data from file: %s", pathname.GetAbsolutePathname().c_str());
        return 0;
    }

    SafeRelease(fp);
    return bytes;
}

bool FileSystem::ReadFileContents(const FilePath& pathname, Vector<uint8>& buffer)
{
    ScopedPtr<File> fp(File::Create(pathname, File::OPEN | File::READ));
    if (!fp)
    {
        Logger::Error("Failed to open file: %s", pathname.GetAbsolutePathname().c_str());
        return false;
    }

    uint32 fileSize = static_cast<uint32>(fp->GetSize());
    buffer.resize(fileSize);
    uint32 dataRead = fp->Read(buffer.data(), fileSize);

    if (dataRead != fileSize)
    {
        Logger::Error("Failed to read data from file: %s", pathname.GetAbsolutePathname().c_str());
        return false;
    }

    return true;
}

void FileSystem::Mount(const FilePath& archiveName, const String& attachPath)
{
    DVASSERT(!attachPath.empty());

    if (!IsMounted(archiveName))
    {
        ResourceArchiveItem item;
        item.attachPath = attachPath;
        item.archive.reset(new ResourceArchive(archiveName));
        item.archiveFilePath = archiveName;

        {
            LockGuard<Mutex> lock(accessArchiveMap);
            resArchiveMap.emplace(archiveName.GetBasename(), std::move(item));
        }
    }
}

void FileSystem::Unmount(const FilePath& arhiveName)
{
    LockGuard<Mutex> lock(accessArchiveMap);
    resArchiveMap.erase(arhiveName.GetBasename());
}

bool FileSystem::IsMounted(const FilePath& archiveName) const
{
    LockGuard<Mutex> lock(accessArchiveMap);
    return resArchiveMap.find(archiveName.GetBasename()) != end(resArchiveMap);
}

int32 FileSystem::Spawn(const String& command)
{
    int32 retCode = 0;
#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_LINUX__)
    retCode = std::system(command.c_str());
#elif defined(__DAVAENGINE_WINDOWS__)

    /* std::system calls "start" command from Windows command line
	Start help:
	Starts a separate window to run a specified program or command.

	START ["title"] [/D path] [/I] [/MIN] [/MAX] [/SEPARATE | /SHARED]
	[/LOW | /NORMAL | /HIGH | /REALTIME | /ABOVENORMAL | /BELOWNORMAL]
	[/NODE <NUMA node>] [/AFFINITY <hex affinity mask>] [/WAIT] [/B]
	[command/program] [parameters]

	If we use "" for path to executable, start resolves it as title. So we need to specify call of start
	http://stackoverflow.com/questions/5681055/how-do-i-start-a-windows-program-with-spaces-in-the-path-from-perl

	*/

    String startString = "start \"\" /WAIT " + command;
    retCode = ::system(startString.c_str());
#endif

    if (retCode != 0)

    {
        Logger::Warning("[FileSystem::Spawn] command (%s) has return code (%d)", command.c_str(), retCode);
    }
    return retCode;
}

void FileSystem::MarkFolderAsNoMedia(const FilePath& folder)
{
#if defined(__DAVAENGINE_ANDROID__)
    // for android we create .nomedia file to say to the OS that this directory have no media content and exclude it from index
    File* nomedia = File::Create(folder + ".nomedia", File::WRITE | File::CREATE);
    SafeRelease(nomedia);
#endif
}

bool FileSystem::CompareTextFiles(const FilePath& filePath1, const FilePath& filePath2)
{
    ScopedPtr<File> f1(File::Create(filePath1, File::OPEN | File::READ));
    ScopedPtr<File> f2(File::Create(filePath2, File::OPEN | File::READ));

    if (nullptr == static_cast<File*>(f1) || nullptr == static_cast<File*>(f2))
    {
        Logger::Error("Couldn't compare file %s and file %s, can't open", filePath1.GetAbsolutePathname().c_str(), filePath2.GetAbsolutePathname().c_str());
        return false;
    }

    String tmpStr1;
    bool end1;
    String tmpStr2;
    bool end2;
    bool feof1 = false;
    bool feof2 = false;

    do
    {
        tmpStr1 = f1->ReadLine();
        end1 = HasLineEnding(f1);

        tmpStr2 = f2->ReadLine();
        end2 = HasLineEnding(f2);

        // if one file have no line ending and another - have - we tryes to compare binary file with text file
        // if we have no line endings - then we tryes to compare binary files - comparision is correct
        if (end1 != end2)
        {
            return false;
        }

        if (tmpStr1.size() != tmpStr2.size() && 0 != tmpStr1.compare(tmpStr2))
        {
            return false;
        }
        feof1 = f1->IsEof();
        feof2 = f2->IsEof();

    } while (!feof1 && !feof2);

    return (feof1 == feof2);
}

bool FileSystem::HasLineEnding(File* f)
{
    bool isHave = false;
    uint8 prevChar;
    f->Seek(-1, File::SEEK_FROM_CURRENT);
    if (1 == f->Read(&prevChar, 1))
    {
        isHave = '\n' == prevChar;
    }

    // make sure that we have eof if it was before HasLineEnding call
    if (1 == f->Read(&prevChar, 1))
    {
        f->Seek(-1, File::SEEK_FROM_CURRENT);
    }
    return isHave;
}

bool FileSystem::CompareBinaryFiles(const FilePath& filePath1, const FilePath& filePath2)
{
    ScopedPtr<File> f1(File::Create(filePath1, File::OPEN | File::READ));
    ScopedPtr<File> f2(File::Create(filePath2, File::OPEN | File::READ));

    if (nullptr == static_cast<File*>(f1) || nullptr == static_cast<File*>(f2))
    {
        Logger::Error("Couldn't compare file %s and file %s, can't open", filePath1.GetAbsolutePathname().c_str(), filePath2.GetAbsolutePathname().c_str());
        return false;
    }

    const uint32 bufferSize = 16 * 1024 * 1024;

    uint8* buffer1 = new uint8[bufferSize];
    uint8* buffer2 = new uint8[bufferSize];

    SCOPE_EXIT
    {
        SafeDelete(buffer1);
        SafeDelete(buffer2);
    };

    bool res = false;

    do
    {
        uint32 actuallyRead1 = f1->Read(buffer1, bufferSize);
        uint32 actuallyRead2 = f2->Read(buffer2, bufferSize);

        if (actuallyRead1 != actuallyRead2)
        {
            res = false;
            break;
        }

        res = 0 == Memcmp(buffer1, buffer2, actuallyRead1);
    } while (res && !f1->IsEof() && !f2->IsEof());

    if (res && f1->IsEof() != f2->IsEof())
    {
        res = false;
    }

    return res;
}

// deprecated method
bool FileSystem::GetFileSize(const FilePath& path, uint32& size)
{
    uint64 fullSize = 0;
    if (GetFileSize(path, fullSize))
    {
        if (fullSize > std::numeric_limits<uint32>::max())
        {
            DAVA_THROW(DAVA::Exception, "size of file: more 4Gb use 64 bit version");
        }
        size = static_cast<uint32>(fullSize);
        return true;
    }
    return false;
}

bool FileSystem::GetFileSize(const FilePath& path, uint64& size)
{
    // TODO we can implement it much faster with posix or winapi and
    // android AssetsManager
    ScopedPtr<File> file(File::Create(path, File::OPEN | File::READ));
    if (file)
    {
        size = file->GetSize();
        return true;
    }

    return false;
}

bool FileSystem::Exists(const FilePath& filePath) const
{
    const FilePath::ePathType pathType = filePath.GetType();
    if (pathType == FilePath::PATH_IN_MEMORY || pathType == FilePath::PATH_EMPTY)
    {
        return false;
    }

    if (filePath.IsDirectoryPathname())
    {
        return IsDirectory(filePath);
    }

    return IsFile(filePath);
}

bool FileSystem::ExistsInAndroidAssets(const FilePath& path) const
{
#if defined(__DAVAENGINE_ANDROID__)
    {
        // We do not use Mutex here because call const methods and only search for file, no extracting from minizip
        AssetsManagerAndroid* assetsManager = AssetsManagerAndroid::Instance();
        DVASSERT(assetsManager, "Need to create AssetsManagerAndroid before checking files in android APK");

        String relativePath = path.GetAbsolutePathname();
        return assetsManager->HasFile(relativePath);
    }
#else
    return false;
#endif // __DAVAENGINE_ANDROID__
}

bool FileSystem::RecursiveCopy(const DAVA::FilePath& src, const DAVA::FilePath& dst)
{
    DVASSERT(src.IsDirectoryPathname() && dst.IsDirectoryPathname());
    DVASSERT(dst.GetType() != FilePath::PATH_IN_RESOURCES);

    CreateDirectory(dst, true);

    bool retCode = true;
    ScopedPtr<FileList> fileList(new FileList(src));
    for (uint32 i = 0; i < fileList->GetCount(); ++i)
    {
        if (fileList->IsDirectory(i))
        {
            if (!fileList->IsNavigationDirectory(i))
            {
                retCode = retCode && RecursiveCopy(fileList->GetPathname(i), dst + (fileList->GetFilename(i) + "/"));
            }
        }
        else
        {
            const FilePath destinationPath = dst + fileList->GetFilename(i);
            if (!CopyFile(fileList->GetPathname(i), destinationPath, false))
            {
                retCode = false;
            }
        }
    }
    return retCode;
}

void FileSystem::SetFilenamesTag(const String& newTag)
{
    filenamesTag = newTag;
}

const String& FileSystem::GetFilenamesTag() const
{
    return filenamesTag;
}

void FileSystem::SetDelegate(FileSystemDelegate* delegate)
{
    fsDelegate = delegate;
}

FileSystemDelegate* FileSystem::GetDelegate() const
{
    return fsDelegate;
}
}
