#include "DLCManager/Private/PackRequest.h"
#include "DLCManager/Private/DLCManagerImpl.h"
#include "FileSystem/Private/PackMetaData.h"
#include "FileSystem/FileSystem.h"
#include "Utils/CRC32.h"
#include "Logger/Logger.h"
#include "Compression/Compressor.h"

#include <numeric>

namespace DAVA
{
const String extPart(".part");

PackRequest::PackRequest(const String& packName)
    : packManager(nullptr)
    , requestedPackName(packName)
    , delayedRequest(false)
    , fileRequestsInitialized(true)
    , isDownloaded(true)
{
    // local pack creation
}

PackRequest::PackRequest(DLCManagerImpl& packManager_, const String& pack_)
    : packManager(&packManager_)
    , requestedPackName(pack_)
    , delayedRequest(true)
{
}

PackRequest::PackRequest(DLCManagerImpl& packManager_, const String& pack_, Vector<uint32> fileIndexes_)
    : packManager(&packManager_)
    , requestedPackName(pack_)
    , delayedRequest(false)
{
    SetFileIndexes(move(fileIndexes_));
}

void PackRequest::CancelCurrentDownloadRequests()
{
    DVASSERT(Thread::IsMainThread());

    DLCDownloader& downloader = packManager->GetDownloader();

    for (FileRequest& r : requests)
    {
        if (r.task != nullptr)
        {
            downloader.RemoveTask(r.task);
            r.task = nullptr;
            r.status = CheckLocalFile;
        }
    }
}

PackRequest::~PackRequest()
{
    DVASSERT(Thread::IsMainThread());
    // in local packs packManager can be null
    if (packManager != nullptr)
    {
        CancelCurrentDownloadRequests();
        packManager = nullptr;
    }
    requests.clear();
    fileIndexes.clear();
    requestedPackName.clear();
    delayedRequest = false;
}

void PackRequest::Start()
{
    Update();
    StartLoading();
}

void PackRequest::Stop()
{
    isLoadingStarted = false;
    CancelCurrentDownloadRequests();
}

const String& PackRequest::GetRequestedPackName() const
{
    return requestedPackName;
}

const Vector<uint32>& PackRequest::GetDirectDependencies() const
{
    if (packManager->IsInitialized())
    {
        const PackMetaData& packMetaData = packManager->GetRemoteMeta();
        return packMetaData.GetPackDependencyIndexes(requestedPackName);
    }
    DAVA_THROW(Exception, "Error! Can't get pack dependencies before initialization is finished");
}
/** return size of files within this request without dependencies */
uint64 PackRequest::GetSize() const
{
    uint64 allFilesSize = 0;
    const auto& files = packManager->GetPack().filesTable.data.files;
    for (uint32 fileIndex : fileIndexes)
    {
        DVASSERT(fileIndex < files.size());
        const auto& fileInfo = files[fileIndex];
        allFilesSize += fileInfo.compressedSize;
    }
    return allFilesSize;
}
/** recalculate current downloaded size without dependencies */
uint64 PackRequest::GetDownloadedSize() const
{
    if (totalDownloadedSize != 0)
    {
        return totalDownloadedSize;
    }

    const uint64 requestsSize = std::accumulate(begin(requests), end(requests), uint64(0), [](uint64 sum, const FileRequest& r) {
        // count ONLY ready file request to eliminate thresholding of current downloading progress
        if (r.status == Ready)
        {
            return sum + r.downloadedFileSize;
        }
        return sum;
    });
    return requestsSize;
}

DLCManager& DLCManager::IRequest::GetDLCManager() const
{
    DAVA_THROW(DAVA::Exception, "implement it for your DLCManager::IRequest");
}

DLCManager& PackRequest::GetDLCManager() const
{
    return *packManager;
}

/** return true when all files loaded and ready */
bool PackRequest::IsDownloaded() const
{
    if (isDownloaded)
    {
        return true;
    }

    if (delayedRequest)
    {
        return false;
    }

    if (!fileRequestsInitialized)
    {
        return false;
    }

    if (!packManager->IsInitialized())
    {
        return false;
    }

    for (const FileRequest& r : requests)
    {
        if (r.status != Ready)
        {
            return false;
        }
    }

    if (packManager->IsInQueue(this))
    {
        if (!packManager->IsTop(this))
        {
            // wait for dependencies to download first
            return false;
        }
    }

    auto checkDependencies = [&]() -> bool
    {
        const PackMetaData& packMetaData = packManager->GetRemoteMeta();
        const uint32 packIndex = packMetaData.GetPackIndex(requestedPackName);
        const Vector<uint32>& dependencies = packMetaData.GetDependencies(packIndex);
        for (const uint32 depIndex : dependencies)
        {
            const PackMetaData::PackInfo& packInfo = packManager->GetRemoteMeta().GetPackInfo(depIndex);
            const PackRequest* const depRequest = packManager->FindRequest(packInfo.packName);
            if (depRequest == nullptr || depRequest->IsDownloaded() == false)
            {
                packManager->GetLog() << "current pack: " << requestedPackName << " dependent pack is not loaded: " << packInfo.packName << std::endl;
                return false;
            }
        }
        return true;
    };

    DVASSERT(checkDependencies() == true); // do it only in debug

    isDownloaded = true; // cache positive result
    return isDownloaded;
}

void PackRequest::Finalize()
{
    DVASSERT(isDownloaded);
    DVASSERT(Thread::IsMainThread());

    totalDownloadedSize = GetDownloadedSize();
    requests.clear();
    requests.shrink_to_fit();
}

void PackRequest::SetFileIndexes(Vector<uint32> fileIndexes_)
{
    fileIndexes = std::move(fileIndexes_);
    delayedRequest = false;
}

bool PackRequest::IsSubRequest(const PackRequest* other) const
{
    const auto& meta = packManager->GetRemoteMeta();
    const uint32 thisPackIndex = meta.GetPackIndex(requestedPackName);
    const uint32 otherPackIndex = meta.GetPackIndex(other->requestedPackName);
    return meta.HasDependency(thisPackIndex, otherPackIndex);
}

void PackRequest::StartLoading()
{
    if (!isLoadingStarted)
    {
        isLoadingStarted = true;
        packManager->requestStartLoading.Emit(*this);
    }
}

void PackRequest::InitializeFileRequests()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &packManager->profiler);

    DVASSERT(fileRequestsInitialized == false);
    DVASSERT(requests.empty());

    requests.reserve(fileIndexes.size());

    const String& url = packManager->GetSuperPackUrl();

    for (size_t requestIndex = 0; requestIndex < fileIndexes.size(); ++requestIndex)
    {
        uint32 fileIndex = fileIndexes.at(requestIndex);
        const auto& fileInfo = packManager->GetPack().filesTable.data.files.at(fileIndex);
        String relativePath = packManager->GetRelativeFilePath(fileIndex);
        FilePath localPath = packManager->GetLocalPacksDirectory() + relativePath + extDvpl + extPart;

        requests.emplace_back(localPath,
                              url,
                              fileIndex,
                              fileInfo.compressedCrc32,
                              fileInfo.startPosition,
                              fileInfo.compressedSize,
                              fileInfo.originalSize,
                              nullptr,
                              fileInfo.type,
                              CheckLocalFile);
    }

    fileRequestsInitialized = true;
}

bool PackRequest::Update()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &packManager->profiler);

    DVASSERT(Thread::IsMainThread());
    DVASSERT(packManager->IsInitialized());

    bool needFireUpdateSignal = false;

    if (!fileRequestsInitialized)
    {
        InitializeFileRequests();
        StartLoading();
    }

    if (!IsDownloaded())
    {
        needFireUpdateSignal = UpdateFileRequests();
    }

    return needFireUpdateSignal;
}

PackRequest::FileRequest::FileRequest(FilePath localFile_,
                                      String url_,
                                      uint32 fileIndex_,
                                      uint32 hashFromMeta_,
                                      uint64 startLoadingPos_,
                                      uint64 sizeOfCompressedFile_,
                                      uint64 sizeOfUncompressedFile_,
                                      DLCDownloader::ITask* task_,
                                      Compressor::Type compressionType_,
                                      Status status_)
    : localFile(localFile_)
    , url(url_)
    , fileIndex(fileIndex_)
    , compressedCrc32(hashFromMeta_)
    , startLoadingPos(startLoadingPos_)
    , sizeOfCompressedFile(sizeOfCompressedFile_)
    , sizeOfUncompressedFile(sizeOfUncompressedFile_)
    , downloadedFileSize(0)
    , task(task_)
    , compressionType(compressionType_)
    , status(status_)
{
}

PackRequest::FileRequest::~FileRequest()
{
    // just for safe clear all fields
    localFile = FilePath();
    url.clear();
    fileIndex = 0;
    compressedCrc32 = 0;
    startLoadingPos = 0;
    sizeOfCompressedFile = 0;
    sizeOfUncompressedFile = 0;
    downloadedFileSize = 0;
    task = nullptr;
    compressionType = Compressor::Type::Lz4HC;
    status = CheckLocalFile;
}

bool PackRequest::CheckLocalFileState(FileSystem* fs, FileRequest& fileRequest)
{
    if (packManager->IsFileReady(fileRequest.fileIndex))
    {
        fileRequest.downloadedFileSize = fileRequest.sizeOfCompressedFile;
        fileRequest.status = Ready;
        return true;
    }

    fileRequest.status = LoadingPackFile;
    return false;
}

bool operator==(const DLCDownloader::TaskError& errLeft, const DLCDownloader::TaskError& errRight)
{
    return errLeft.errorHappened == errRight.errorHappened &&
    errLeft.curlErr == errRight.curlErr &&
    errLeft.curlMErr == errRight.curlMErr &&
    strcmp(errLeft.errStr, errRight.errStr) == 0 &&
    errLeft.fileErrno == errRight.fileErrno &&
    errLeft.httpCode == errRight.httpCode &&
    errLeft.fileLine == errRight.fileLine;
}

bool operator!=(const DLCDownloader::TaskError& errLeft, const DLCDownloader::TaskError& errRight)
{
    return !(errLeft == errRight);
}

bool PackRequest::CheckLoadingStatusOfFileRequest(FileRequest& fileRequest, DLCDownloader& dm, const String& dstPath)
{
    if (fileRequest.task == nullptr)
    {
        return false;
    }

    DLCDownloader::TaskStatus status = dm.GetTaskStatus(fileRequest.task);
    {
        switch (status.state)
        {
        case DLCDownloader::TaskState::JustAdded:
            break;
        case DLCDownloader::TaskState::Downloading:
        {
            if (fileRequest.downloadedFileSize != status.sizeDownloaded)
            {
                fileRequest.downloadedFileSize = status.sizeDownloaded;
                return true;
            }
        }
        break;
        case DLCDownloader::TaskState::Finished:
        {
            dm.RemoveTask(fileRequest.task);
            fileRequest.task = nullptr;
            fileRequest.dvplWriter.reset();

            if (status.error.errorHappened)
            {
                // log same error only once, stop spam
                if (prevTaskError != status.error)
                {
                    packManager->GetLog() << "file_request failed: can't download file: " << dstPath << " status: " << status << std::endl;
                    prevTaskError = status.error;
                }

                if (status.error.curlErr != 0
                    || status.error.curlMErr != 0
                    || status.error.httpCode >= 400)
                {
                    packManager->FireNetworkReady(false);
                }

                if (status.error.fileErrno != 0 && status.error.httpCode < 400)
                {
                    bool fireSignal = packManager->CountError(status.error.fileErrno);
                    if (fireSignal)
                    {
                        String pathname = fileRequest.localFile.GetAbsolutePathname();
                        packManager->error.Emit(DLCManager::ErrorOrigin::FileIO, status.error.fileErrno, pathname);
                    }
                }

                fileRequest.downloadedFileSize = 0;
                fileRequest.status = LoadingPackFile;
                return false;
            }

            fileRequest.downloadedFileSize = status.sizeDownloaded;
            fileRequest.status = Ready;
            DVASSERT(fileRequest.downloadedFileSize == fileRequest.sizeOfCompressedFile);
            packManager->SetFileIsReady(fileRequest.fileIndex, static_cast<uint32>(fileRequest.sizeOfCompressedFile));
            packManager->FireNetworkReady(true);

            return true;
        }
        }
    }
    return false;
}

bool PackRequest::LoadingPackFileState(FileSystem* fs, FileRequest& fileRequest)
{
    DLCDownloader& dm = packManager->GetDownloader();
    String dstPath = fileRequest.localFile.GetAbsolutePathname();
    if (fileRequest.task == nullptr)
    {
        fileRequest.dvplWriter.reset(new DVPLWriter(fileRequest.localFile,
                                                    static_cast<uint32>(fileRequest.sizeOfCompressedFile),
                                                    static_cast<uint32>(fileRequest.sizeOfUncompressedFile),
                                                    fileRequest.compressedCrc32,
                                                    fileRequest.compressionType));

        DLCDownloader::Range range = DLCDownloader::Range(fileRequest.startLoadingPos, fileRequest.sizeOfCompressedFile);
        fileRequest.task = dm.ResumeTask(fileRequest.url, fileRequest.dvplWriter, range);

        if (nullptr == fileRequest.task)
        {
            Logger::Error("can't create task: url: %s, dstPath: %s, range: %lld-%lld", fileRequest.url.c_str(), dstPath.c_str(), fileRequest.sizeOfCompressedFile, fileRequest.startLoadingPos);
            fileRequest.status = CheckLocalFile; // lets start all over again
        }
        return false;
    }

    return CheckLoadingStatusOfFileRequest(fileRequest, dm, dstPath);
}

bool PackRequest::UpdateFileRequests()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &packManager->profiler);

    DVASSERT(Thread::IsMainThread());
    // return true if at least one part file continue downloading
    bool callUpdateSignal = false;

    FileSystem* fs = GetEngineContext()->fileSystem;

    for (FileRequest& fileRequest : requests)
    {
        bool downloadedMore = false;
        switch (fileRequest.status)
        {
        case CheckLocalFile:
        {
            downloadedMore = CheckLocalFileState(fs, fileRequest);
            break;
        }
        case LoadingPackFile:
        {
            downloadedMore = LoadingPackFileState(fs, fileRequest);
            break;
        }
        case Ready:
            break;
        case Error:
            break;
        } // end switch

        if (downloadedMore)
        {
            callUpdateSignal = true;
        }
        if (requests.empty())
        {
            // in case of cancel current download or disable requesting
            break;
        }
    } // end for requests

    // call signal only once during update
    return callUpdateSignal;
}

bool PackRequest::DVPLWriter::OpenFile()
{
    DVASSERT(!fout.is_open());
    // create intermediate directories
    FileSystem* fs = GetEngineContext()->fileSystem;

    FilePath directory = localPath.GetDirectory();
    if (FileSystem::DIRECTORY_CANT_CREATE == fs->CreateDirectory(directory, true))
    {
        const char* err = strerror(errno);
        Logger::Error("can't create output directory: %s errno: %s for file path: %s", directory.GetAbsolutePathname().c_str(), err, localPath.GetStringValue().c_str());
        return false;
    }

    // if file already exist open it read to the end and calculate crc32
    if (fs->IsFile(localPath))
    {
        ScopedPtr<File> f(File::Create(localPath, File::OPEN | File::READ));
        if (!f)
        {
            return false;
        }
        const size_t bufSize = 1024 * 8; // 8k
        char buf[bufSize];

        uint32 n = 0;
        while ((n = f->Read(buf, bufSize)) > 0)
        {
            crc32counter.AddData(buf, n);
        }
    }

    String filePath = localPath.GetAbsolutePathname();
    fout.open(filePath, std::ios::binary | std::ios::out | std::ios::ate);
    return fout.is_open();
}

/** Save next buffer bytes into memory or file, on error return differs from parameter size */
uint64 PackRequest::DVPLWriter::Save(const void* ptr, uint64 size)
{
    if (!fout.is_open())
    {
        if (!OpenFile())
        {
            return 0;
        }
    }

    const char* p = reinterpret_cast<const char*>(ptr);
    std::streamsize s = static_cast<std::streamsize>(size);

    fout.write(p, s);

    if (!fout)
    {
        Logger::Error("failed to write dvpl");
        return 0;
    }

    fout.flush();

    if (!fout)
    {
        Logger::Error("failed to write(flush) dvpl");
        return 0;
    }

    crc32counter.AddData(ptr, static_cast<size_t>(size));

    return size;
}
/** Return current size of saved byte stream, return ```std::numeric_limits<uint64>::max()``` value on error */
uint64 PackRequest::DVPLWriter::GetSeekPos()
{
    if (!fout.is_open())
    {
        if (!OpenFile())
        {
            return std::numeric_limits<uint64>::max();
        }
    }

    std::streamoff pos = fout.tellp();
    uint64 result = static_cast<uint64>(pos);
    return result;
}
/** Truncate file(or buffer) to zero length, return false on error */
bool PackRequest::DVPLWriter::Truncate()
{
    if (fout.is_open())
    {
        std::streamoff pos = fout.tellp();
        if (pos != 0)
        {
            String filePath = localPath.GetAbsolutePathname();
            fout.open(filePath, std::ios::binary | std::ios::out | std::ios::trunc);
        }
        return fout.good();
    }
    return false;
}
/** Close internal resource (file handle, socket, free memory) */
bool PackRequest::DVPLWriter::Close()
{
    if (fout.is_open())
    {
        const uint32 currentFileSize = static_cast<uint32>(fout.tellp());
        const uint32 crc32 = crc32counter.Done();

        FileSystem* fs = GetEngineContext()->fileSystem;

        if (currentFileSize == sizeCompressed &&
            crc32 == crc32Compressed)
        {
            // all nice, now write footer to file, then rename it
            // write 20 bytes LitePack footer
            PackFormat::LitePack::Footer footer = { sizeUncompressed,
                                                    sizeCompressed,
                                                    crc32Compressed,
                                                    compressionType,
                                                    PackFormat::FILE_MARKER_LITE };

            const auto ptr = reinterpret_cast<const char*>(&footer);
            fout.write(ptr, sizeof(footer));
            if (!fout)
            {
                Logger::Error("failed to write footer to dvpl");
                return false;
            }
            fout.close();

            if (!fout)
            {
                Logger::Error("failed to close dvpl");
                return false;
            }
            DVASSERT(localPath.GetExtension() == extPart);

            FilePath newPath(localPath);
            newPath.ReplaceExtension("");

            DVASSERT(newPath.GetExtension() == extDvpl);

            if (!fs->MoveFile(localPath, newPath, true))
            {
                Logger::Error("failed to move file: %s to %s", localPath.GetStringValue().c_str(), newPath.GetStringValue().c_str());
                return false;
            }
        }
        else
        {
            fout.close();
            fs->DeleteFile(localPath);
            // in any case return false if hash not match
            return false;
        }
    }
    return true;
}
/** Check internal state */
bool PackRequest::DVPLWriter::IsClosed() const
{
    return !fout.is_open();
}

} // end namespace DAVA
