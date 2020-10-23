#include "FileSystem/File.h"

#include "Engine/Engine.h"

#include "DLCManager/DLCManager.h"
#include "DLCManager/Private/DLCManagerImpl.h"

#include "FileSystem/DynamicMemoryFile.h"
#include "FileSystem/FileAPIHelper.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/FileSystemDelegate.h"
#include "FileSystem/Private/PackFormatSpec.h"
#include "FileSystem/Private/CheckIOError.h"
#include "FileSystem/ResourceArchive.h"
#include "Engine/Private/Android/AssetsManagerAndroid.h"

#include "Compression/LZ4Compressor.h"
#include "Concurrency/LockGuard.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/Thread.h"
#include "Logger/Logger.h"
#include "Utils/StringFormat.h"
#include "Utils/CRC32.h"

#if defined(__DAVAENGINE_WINDOWS__)
#include <io.h>
#elif defined(__DAVAENGINE_POSIX__)
#include <unistd.h>
#endif

#include <ctime>
#include <sys/stat.h>

namespace DAVA
{
const String extDvpl(".dvpl");

File::~File()
{
    // Though File object is created through Create methods returning nullptr on error
    // pointer should be checked against nullptr as class File can have inheritors
    // which do not initialize file pointer (e.g. DynamicMemoryFile)
    if (file != nullptr)
    {
        int result = fclose(file);
        if (result != 0)
        {
            const String& s = filename.GetStringValue();
            Logger::Error("failed close file: %s", s.c_str());
        }
        file = nullptr;
    }

#ifdef __DAVAENGINE_DEBUG__
    DebugFS::GenErrorOnCloseFailed();
#endif
}

static uint64 GetFilePos(FILE* f)
{
#if defined(__DAVAENGINE_WINDOWS__)
    int64 result = _ftelli64(f);
    DVASSERT(-1 != result);
    return static_cast<uint64>(result);
#else
    off_t result = ftello(f);
    DVASSERT(-1 != result);
    return static_cast<uint64>(result);
#endif
}

static int SetFilePos(FILE* f, int64 position, int32 seekDirection)
{
#if defined(__DAVAENGINE_WINDOWS__)
    return _fseeki64(f, position, seekDirection);
#else
    return fseeko(f, position, seekDirection);
#endif
}

static void LogErrorAndRemoveBadFile(const FilePath& filename, const String& fileNameAbs)
{
    std::stringstream ss;
    DLCManager* dlc = GetEngineContext()->dlcManager;
    if (dlc)
    {
        const DLCManager::FileInfo fileInfo = dlc->GetFileInfo(filename);

        ss << "error: loading file(dvpl): " << fileInfo.relativePathInMeta << '\n' <<
        "pack_name: " << fileInfo.packName << '\n' <<
        "index_of_file_in_meta: " << fileInfo.indexOfFileInMeta << '\n' <<
        "index_of_pack_in_meta: " << fileInfo.indexOfPackInMeta << '\n' <<
        "hash_compressed_in_meta: 0x" << std::hex << fileInfo.hashCompressedInMeta << '\n' <<
        "hash_uncompressed_in_meta: 0x" << std::hex << fileInfo.hashUncompressedInMeta << '\n' <<
        "size_compressed_in_meta: " << std::dec << fileInfo.sizeCompressedInMeta << '\n' <<
        "size_uncompressed_in_meta: " << fileInfo.sizeUncompressedInMeta << '\n' <<
        "is_known_file: " << std::boolalpha << fileInfo.isKnownFile << '\n' <<
        "is_local_file: " << std::boolalpha << fileInfo.isLocalFile << '\n' <<
        "is_remote_file: " << std::boolalpha << fileInfo.isRemoteFile << '\n' <<
        "is_dlc_mng_think_file_ready: " << std::boolalpha << fileInfo.isDlcMngThinkFileReady << '\n';
    }
    else
    {
        ss << "GetEngineContext()->dlcManager is nullptr\n";
    }
    ss << "is_regular_file: " << std::boolalpha << FileAPI::IsRegularFile(fileNameAbs) << '\n'
       << "on disk file_size(+" << sizeof(PackFormat::LitePack::Footer)
       << " byte footer): " << std::dec << FileAPI::GetFileSize(fileNameAbs) << '\n';

    const uint32 hashWithoutFooter = CRC32::ForDVPLFileContent(fileNameAbs);

    ss << "on disk file_compressed_hash: 0x" << std::hex << hashWithoutFooter << '\n';
    // delete bad file (can't decompress)
    const bool isFileRemoved = 0 == FileAPI::RemoveFile(fileNameAbs);
    ss << "remove_file result: " << std::boolalpha << isFileRemoved;

    String str = ss.str();
    Logger::Error("%s", str.c_str());
}

File* File::Create(const FilePath& filename, uint32 attributes)
{
#ifdef __DAVAENGINE_DEBUG__
    if (DebugFS::GenErrorOnOpenOrCreateFailed())
    {
        return nullptr;
    }
#endif

    if (filename.IsDirectoryPathname())
    {
        return nullptr;
    }

    //Tags
    FileSystem* fs = GetEngineContext()->fileSystem;
    FileSystemDelegate* fsDelegate = fs->GetDelegate();
    if (fsDelegate != nullptr)
    { // hooked check: can we continue work with file?
        String path = filename.GetAbsolutePathname();
        if (fsDelegate->CanCreateFile(path, attributes) == false)
        {
            return nullptr;
        }
    }

    if (!(attributes & (WRITE | CREATE | APPEND)) && fs->filenamesTag.empty() == false)
    {
        FilePath taggedFilename = filename;
        String basename = filename.GetBasename();
        String::size_type pointPos = basename.find(".");
        if (pointPos == String::npos)
        { // ... any file
            taggedFilename.ReplaceBasename(basename + fs->filenamesTag);
        }
        else
        { // ... texture.PowerVR_iOS.pvr
            basename.insert(pointPos, fs->filenamesTag);
            taggedFilename.ReplaceBasename(basename);
        }

        File* result = PureCreate(taggedFilename, attributes);
        if (result != nullptr)
        {
            result->filename = filename;
            return result;
        }
    }
    //end of tags

    File* result = PureCreate(filename, attributes);
    if (result != nullptr)
    {
        return result;
    }

    if (!(attributes & (WRITE | CREATE | APPEND)))
    {
        FilePath compressedFile = filename + extDvpl;
        const String fileNameAbs = compressedFile.GetAbsolutePathname();
        if (FileAPI::IsRegularFile(fileNameAbs))
        {
            try
            {
                result = CompressedCreate(compressedFile, attributes);
            }
            catch (std::exception& ex)
            {
                Logger::Error("error: (%s) decompress exception: %s", fileNameAbs.c_str(), ex.what());
            }

            if (result == nullptr)
            {
                LogErrorAndRemoveBadFile(filename, fileNameAbs);
            }
        }
    }
    return result; // easy debug on android(can set breakpoint on nullptr value in eclipse do not remove it)
}

File* File::LoadFileFromMountedArchive(const String& packName, const String& relative)
{
    FileSystem* fs = FileSystem::Instance();
    {
        LockGuard<Mutex> lock(fs->accessArchiveMap);

        auto it = fs->resArchiveMap.find(packName);
        if (it != end(fs->resArchiveMap))
        {
            Vector<uint8> fileContent;
            if (it->second.archive->LoadFile(relative, fileContent))
            {
                return DynamicMemoryFile::Create(std::move(fileContent), READ, "~res:/" + relative);
            }
        }
        return nullptr;
    }
}

bool File::IsFileInMountedArchive(const String& packName, const String& relative)
{
    FileSystem* fs = FileSystem::Instance();
    {
        LockGuard<Mutex> lock(fs->accessArchiveMap);

        auto it = fs->resArchiveMap.find(packName);
        if (it != end(fs->resArchiveMap))
        {
            return it->second.archive->HasFile(relative);
        }
        return false;
    }
}

File* File::CompressedCreate(const FilePath& filename, uint32 attributes)
{
#ifdef __DAVAENGINE_DEBUG__
    if (DebugFS::GenErrorOnOpenOrCreateFailed())
    {
        return nullptr;
    }
#endif
    ScopedPtr<File> f(PureCreate(filename, attributes));

    if (!f)
    {
        return nullptr;
    }

    const auto fileSize = static_cast<uint32>(f->GetSize());
    const auto footerSize = static_cast<uint32>(sizeof(PackFormat::LitePack::Footer));

    if (fileSize < footerSize)
    {
        Logger::Error("compressed file too small: %s", filename.GetAbsolutePathname().c_str());
        return nullptr;
    }

    const int64 footerPos = fileSize - footerSize;

    if (!f->Seek(footerPos, SEEK_FROM_START))
    {
        Logger::Error("can't seek to footer: %s", filename.GetAbsolutePathname().c_str());
        return nullptr;
    }

    PackFormat::LitePack::Footer footer{ 0, 0, 0, Compressor::Type::None, { '\0', '\0', '\0', '\0' } }; // default init with 0

    if (footerSize != f->Read(&footer, sizeof(footer)))
    {
        Logger::Error("can't read footer: %s", filename.GetAbsolutePathname().c_str());
        return nullptr;
    }

    if (PackFormat::FILE_MARKER_LITE != footer.packMarkerLite)
    {
        Logger::Error("file_marker_lite does not match: %s", filename.GetAbsolutePathname().c_str());
        return nullptr;
    }

    if (!f->Seek(0, SEEK_FROM_START))
    {
        Logger::Error("can't seek to begin: %s", filename.GetAbsolutePathname().c_str());
        return nullptr;
    }

    Vector<uint8> compressed(footer.sizeCompressed);

    if (footer.sizeCompressed != f->Read(&compressed[0], footer.sizeCompressed))
    {
        Logger::Error("can't read compressed bytes: %s", filename.GetAbsolutePathname().c_str());
        return nullptr;
    }

    if (footer.type == Compressor::Type::Lz4HC || footer.type == Compressor::Type::Lz4)
    {
        Vector<uint8> uncompressed(footer.sizeUncompressed);

        if (!LZ4HCCompressor().Decompress(compressed, uncompressed))
        {
            Logger::Error("decompress failed on file: %s", filename.GetAbsolutePathname().c_str());
            return nullptr;
        }

        DynamicMemoryFile* file = DynamicMemoryFile::Create(std::move(uncompressed), attributes, filename);
        return file;
    }

    if (footer.type == Compressor::Type::None)
    {
        DynamicMemoryFile* file = DynamicMemoryFile::Create(std::move(compressed), attributes, filename);
        return file;
    }

    Logger::Error("incorrect compression type: %d file:", static_cast<int32>(footer.type), filename.GetAbsolutePathname().c_str());
    return nullptr;
}

#ifdef __DAVAENGINE_ANDROID__
static File* CreateFromAPK(const FilePath& filePath, uint32 attributes)
{
    static Mutex mutex;

    LockGuard<Mutex> guard(mutex);

    AssetsManagerAndroid* assetsManager = AssetsManagerAndroid::Instance();
    DVASSERT(assetsManager, "[CreateFromAPK] Need to create AssetsManager before loading files");

    Vector<uint8> data;
    if (!assetsManager->LoadFile(filePath.GetAbsolutePathname(), data))
    {
        return nullptr;
    }

    return DynamicMemoryFile::Create(std::move(data), attributes, filePath);
}
#endif // __DAVAENGINE_ANDROID__

File* File::PureCreate(const FilePath& filePath, uint32 attributes)
{
    FILE* file = nullptr;
    uint64 size = 0;
    String path = filePath.GetAbsolutePathname();

    if ((attributes & File::OPEN) && (attributes & File::READ))
    {
        if (attributes & File::WRITE)
        {
            file = FileAPI::OpenFile(path, "r+b");
        }
        else
        {
            file = FileAPI::OpenFile(path, "rb");
        }

        if (!file)
        {
#ifdef __DAVAENGINE_ANDROID__
            bool isFileExistOnRealFS = FileAPI::IsRegularFile(path);

            if (isFileExistOnRealFS)
            {
                int32 openFileAttempt = 1;
                while (!file && (openFileAttempt++ <= 10))
                {
                    if (attributes & File::WRITE)
                    {
                        file = FileAPI::OpenFile(path, "r+b");
                    }
                    else
                    {
                        file = FileAPI::OpenFile(path, "rb");
                    }

                    if (!file)
                    {
                        Logger::Error("can't open existing file: %s attempt: %d, errno: %s",
                                      path.c_str(), openFileAttempt, strerror(errno));
                        Thread::Sleep(100);
                    }
                } // end while
            }

            if (!file)
            {
                File* fromAPK = CreateFromAPK(filePath, attributes);
                return fromAPK; // simpler debugging on android
            }
#else
#ifdef __DAVAENGINE_DEBUG__
// this is a last place where we search for file, so help
// developers a little and add some logs
// String p = UTF8Utils::EncodeToUTF8(path);
// Logger::Error("can't open: %s, cause: %s", p.c_str(), std::strerror(errno));
#endif
            return nullptr;
#endif
        }
        if (0 != SetFilePos(file, 0, SEEK_END))
        {
            Logger::Error("fseek end error");
        }

        size = GetFilePos(file);

        if (0 != SetFilePos(file, 0, SEEK_SET))
        {
            Logger::Error("fseek set error");
        }
    }
    else if ((attributes & File::CREATE) && (attributes & File::WRITE))
    {
        file = FileAPI::OpenFile(path, "wb");
        if (!file)
        {
            return nullptr;
        }
    }
    else if ((attributes & File::APPEND) && (attributes & File::WRITE))
    {
        file = FileAPI::OpenFile(path, "ab");
        if (!file)
        {
            return nullptr;
        }
        if (0 != SetFilePos(file, 0, SEEK_END))
        {
            Logger::Error("fseek set error");
        }
        size = GetFilePos(file);
    }
    else
    {
        return nullptr;
    }

    File* fileInstance = new File();
    fileInstance->filename = filePath;
    fileInstance->size = size;
    fileInstance->file = file;
    return fileInstance;
}

const FilePath& File::GetFilename()
{
    return filename;
}

uint32 File::Write(const void* pointerToData, uint32 dataSize)
{
#ifdef __DAVAENGINE_DEBUG__
    if (DebugFS::GenErrorOnWriteFailed())
    {
        return 0;
    }
#endif
// TODO better write same code on all platforms, fwrite can return buffered num bytes, we have to fflush(file) == 0 to guarantee lSize is correct if (fflush(file) != 0) - write failed
#if defined(__DAVAENGINE_ANDROID__)
    uint32 posBeforeWrite = GetPos();
#endif

    //! Do not change order fread return not bytes -- items
    uint32 lSize = static_cast<uint32>(fwrite(pointerToData, 1, dataSize, file));

#if defined(__DAVAENGINE_ANDROID__)
    //for Android value returned by 'fwrite()' is incorrect in case of full disk, that's why we calculate 'lSize' using 'GetPos()'
    lSize = GetPos() - posBeforeWrite;
#endif

    size += lSize;

    return lSize;
}

uint32 File::Read(void* pointerToData, uint32 dataSize)
{
#ifdef __DAVAENGINE_DEBUG__
    if (DebugFS::GenErrorOnReadFailed())
    {
        return 0;
    }
#endif
    //! Do not change order (1, dataSize), cause fread return count of size(2nd param) items
    //! May be performance issues
    const size_t result = fread(pointerToData, 1, static_cast<size_t>(dataSize), file);
    if (result != dataSize)
    {
        if (ferror(file))
        {
            Logger::Error("file read i/o error: %zu(expected: %u) bytes from file: %s errno: %s",
                          result, dataSize, filename.GetStringValue().c_str(), std::strerror(errno));
        }
        else
        {
            // just EOF do nothing
        }
    }
    return static_cast<uint32>(result);
}

uint32 File::ReadString(char8* destinationBuffer, uint32 destinationBufferSize)
{
    uint32 writeIndex = 0;
    uint8 currentChar = 0;

    if (destinationBufferSize > 0)
    {
        while (Read(&currentChar, 1) > 0)
        {
            if (writeIndex < destinationBufferSize)
            {
                destinationBuffer[writeIndex] = currentChar;
                writeIndex++;
            }
            else
            {
                currentChar = 0;
                Logger::Warning("File::ReadString buffer size is too small for this string.");
            }

            if (currentChar == 0)
            {
                writeIndex--;
                break;
            }
        }

        destinationBuffer[writeIndex] = 0;
    }

    return writeIndex;
}

uint32 File::ReadString(String& destinationString)
{
    uint32 writeIndex = 0;
    uint8 currentChar = 0;

    while (!IsEof() && Read(&currentChar, 1) != 0)
    {
        if (0 != currentChar)
        {
            destinationString += currentChar;
            writeIndex++;
        }
        else
        {
            break;
        }
    }
    return writeIndex - 1;
}

uint32 File::ReadLine(void* pointerToData, uint32 bufferSize)
{
    uint32 ret = 0;

    if (bufferSize > 0)
    {
        uint8* inPtr = reinterpret_cast<uint8*>(pointerToData);
        while (!IsEof() && bufferSize > 1)
        {
            uint8 nextChar;
            if (GetNextChar(&nextChar))
            {
                *inPtr = nextChar;
                inPtr++;
                bufferSize--;
            }
            else
            {
                break;
            }
        }
        *inPtr = 0;
        inPtr++;
        ret = static_cast<uint32>(inPtr - reinterpret_cast<uint8*>(pointerToData));
    }

    return ret;
}

String File::ReadLine()
{
    String destinationString;
    while (!IsEof())
    {
        uint8 nextChar;
        if (GetNextChar(&nextChar))
        {
            destinationString += nextChar;
        }
        else
        {
            break;
        }
    }
    return destinationString;
}

bool File::GetNextChar(uint8* nextChar)
{
    uint64 actuallyRead = Read(nextChar, 1);
    if (actuallyRead != 1)
    {
        //seems IsEof()
        return false;
    }

    if (0 == *nextChar)
    {
        // 0 terminated string
        return false;
    }
    else if ('\r' == *nextChar)
    {
        // we don't need to return \r as a charracter
        return GetNextChar(nextChar);
    }
    else if ('\n' == *nextChar)
    {
        // there was a last charracter in string ended by \n, then we cannot read more
        return false;
    }
    else
    {
        // some regular charracter readed
        return true;
    }
}

uint64 File::GetPos() const
{
#ifdef __DAVAENGINE_DEBUG__
    if (DebugFS::GenErrorOnSeekFailed())
    {
        return std::numeric_limits<uint64>::max();
    }
#endif
    return GetFilePos(file);
}

uint64 File::GetSize() const
{
    return size;
}

bool File::Seek(int64 position, eFileSeek seekType)
{
#ifdef __DAVAENGINE_DEBUG__
    if (DebugFS::GenErrorOnSeekFailed())
    {
        return false;
    }
#endif
    int realSeekType = 0;
    switch (seekType)
    {
    case SEEK_FROM_START:
        realSeekType = SEEK_SET;
        break;
    case SEEK_FROM_CURRENT:
        realSeekType = SEEK_CUR;
        break;
    case SEEK_FROM_END:
        realSeekType = SEEK_END;
        break;
    default:
        DVASSERT(0 && "Invalid seek type");
        break;
    }

    return 0 == SetFilePos(file, position, realSeekType);
}

bool File::Flush()
{
#ifdef __DAVAENGINE_DEBUG__
    if (DebugFS::GenErrorOnWriteFailed())
    {
        return false;
    }
#endif
    return 0 == fflush(file);
}

bool File::IsEof() const
{
    return (feof(file) != 0);
}

bool File::Truncate(uint64 size)
{
#ifdef __DAVAENGINE_DEBUG__
    if (DebugFS::GenErrorOnTruncateFailed())
    {
        return false;
    }
#endif
#if defined(__DAVAENGINE_WINDOWS__)
    return (0 == _chsize(_fileno(file), static_cast<long>(size)));
#elif defined(__DAVAENGINE_POSIX__)
    return (0 == ftruncate(fileno(file), size));
#else
#error No implementation for current platform
    return false;
#endif
}

bool File::WriteString(const String& strtowrite, bool shouldNullBeWritten)
{
    const char* str = strtowrite.c_str();
    uint32 null = (shouldNullBeWritten) ? (1) : (0);
    return (Write(str, static_cast<uint32>(strtowrite.length() + null)) == strtowrite.length() + null);
}

bool File::WriteNonTerminatedString(const String& strtowrite)
{
    const char* str = strtowrite.c_str();
    return (Write(str, static_cast<uint32>(strtowrite.length())) == strtowrite.length());
}

bool File::WriteLine(const String& string)
{
    uint32 written = 0;
    const char* str = string.c_str();
    const char* endLine = "\r\n";
    uint32 endLength = static_cast<uint32>(strlen(endLine));
    uint32 strLength = static_cast<uint32>(string.length());

    written += Write(str, strLength);
    written += Write(endLine, endLength);

    return (written == strLength + endLength);
}

String File::GetModificationDate(const FilePath& filePathname)
{
    String realPathname = filePathname.GetAbsolutePathname();

    struct stat fileInfo = { 0 };
    int32 ret = stat(realPathname.c_str(), &fileInfo);
    if (0 == ret)
    {
#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)
        tm* utcTime = gmtime(&fileInfo.st_mtimespec.tv_sec);
#else
        tm* utcTime = gmtime(&fileInfo.st_mtime);
#endif
        return String(Format("%04d.%02d.%02d %02d:%02d:%02d",
                             utcTime->tm_year + 1900, utcTime->tm_mon + 1, utcTime->tm_mday,
                             utcTime->tm_hour, utcTime->tm_min, utcTime->tm_sec));
    }
    return String("");
}
}
