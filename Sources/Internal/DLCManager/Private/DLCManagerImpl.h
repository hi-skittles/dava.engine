#pragma once

#include <fstream>

#include "DLCManager/DLCManager.h"
#include "DLCManager/DLCDownloader.h"
#include "DLCManager/Private/RequestManager.h"
#include "DLCManager/Private/PackRequest.h"
#include "DLCManager/Private/DebugGestureListener.h"
#include "FileSystem/FilePath.h"
#include "FileSystem/Private/PackFormatSpec.h"
#include "FileSystem/Private/PackMetaData.h"
#include "Functional/Function.h"
#include "Concurrency/Semaphore.h"
#include "Concurrency/Thread.h"
#include "Engine/Engine.h"
#include "Engine/EngineSettings.h"
#include "Debug/ProfilerCPU.h"

namespace DAVA
{
class MemoryBufferWriter final : public DLCDownloader::IWriter
{
public:
    MemoryBufferWriter(void* buff, size_t size)
    {
        DVASSERT(buff != nullptr);

        start = static_cast<char*>(buff);
        current = start;
        end = start + size;
    }

    uint64 Save(const void* ptr, uint64 size) override
    {
        uint64 space = SpaceLeft();

        if (size > space)
        {
            memcpy(current, ptr, static_cast<size_t>(space));
            current += space;
            return space;
        }

        memcpy(current, ptr, static_cast<size_t>(size));
        current += size;
        return size;
    }

    uint64 GetSeekPos() override
    {
        return current - start;
    }

    bool Truncate() override
    {
        current = start;
        return true;
    }

    bool Close() override
    {
        start = nullptr;
        current = nullptr;
        end = nullptr;
        return true;
    }

    bool IsClosed() const override
    {
        return current == nullptr;
    }

    uint64 SpaceLeft() const
    {
        return end - current;
    }

private:
    char* start = nullptr;
    char* current = nullptr;
    char* end = nullptr;
};

class DLCManagerImpl final : public DLCManager
{
public:
    enum class InitState : uint32
    {
        Starting, //!< before any initialization code state
        LoadingRequestAskFooter, //!< connect to server superpack.dvpk for footer block
        LoadingRequestGetFooter, //!< download footer and parse it, find out filetable block size and position
        LoadingRequestAskFileTable, //!< start loading filetable block from superpack.dvpk
        LoadingRequestGetFileTable, //!< download filetable and fill info about every file on server superpack.dvpk
        CalculateLocalDBHashAndCompare, //!< check if existing local DB hash match with remote DB on server, go to LoadingPacksDataFromLocalMeta if match
        LoadingRequestAskMeta, //!< start loading DB from server
        LoadingRequestGetMeta, //!< download DB and check it's hash
        UnpakingDB, //!< unpack DB from zip
        LoadingPacksDataFromLocalMeta, //!< open local DB and build pack index for all packs
        WaitScanThreadToFinish, //!< wait till finish scanning of downloaded .dvpl files
        MoveDeleyedRequestsToQueue, //!< mount all local packs downloaded and not mounted later
        Ready, //!< starting from this state client can call any method, second initialize will work too
        Offline, //!< server not accessible, retry initialization after Hints::retryConnectMilliseconds

        State_COUNT
    };

    static const String& ToString(InitState state);

    explicit DLCManagerImpl(Engine* engine_);
    ~DLCManagerImpl();
    void Initialize(const FilePath& dirToDownloadPacks_,
                    const String& urlToServerSuperpack_,
                    const Hints& hints_) final;

    void Deinitialize() final;

    bool IsInitialized() const final;

    InitState GetInternalInitState() const;

    InitStatus GetInitStatus() const final;

    const String& GetLastErrorMessage() const;

    bool IsRequestingEnabled() const final;

    void SetRequestingEnabled(bool value) final;

    void Update(float frameDelta, bool inBackground);

    bool IsPackDownloaded(const String& packName) const final;

    uint64 GetPackSize(const String& packName) const final;

    const IRequest* RequestPack(const String& requestedPackName) final;

    PackRequest* FindRequest(const String& requestedPackName) const;

    bool IsPackInQueue(const String& packName) const final;

    bool IsAnyPackInQueue() const final;

    bool IsKnownFile(const FilePath& relativeFileName) const final;

    void SetRequestPriority(const IRequest* request) final;

    void RemovePack(const String& packName) final;

    void ResetQueue() final;

    Progress GetProgress() const final;

    Progress GetPacksProgress(const Vector<String>& packNames) const final;

    Info GetInfo() const final;

    FileInfo GetFileInfo(const FilePath& path) const final;

    const FilePath& GetLocalPacksDirectory() const;

    const String& GetSuperPackUrl() const;

    uint32 GetServerFooterCrc32() const;

    String GetRelativeFilePath(uint32 fileIndex);

    const Hints& GetHints() const;

    const PackMetaData& GetRemoteMeta() const;

    const PackMetaData& GetLocalMeta() const;

    bool HasLocalMeta() const;

    bool HasRemoteMeta() const;

    const PackFormat::PackFile& GetPack() const;

    // use only after initialization
    bool IsFileReady(size_t fileIndex) const;

    void SetFileIsReady(size_t fileIndex, uint32 compressedSize);

    bool IsInQueue(const PackRequest* request) const;

    bool IsTop(const PackRequest* request) const;

    std::ostream& GetLog() const;

    DLCDownloader& GetDownloader() const;

    bool CountError(int32 errCode);
    void FireNetworkReady(bool nextState);

    uint32 instanceIndex = 0;

    ProfilerCPU profiler;

private:
    // initialization state functions
    void AskFooter();
    void GetFooter();
    void AskFileTable();
    void GetFileTable();
    void CompareLocalMetaWitnRemoteHash();
    void AskServerMeta();
    void GetServerMeta();
    void ParseMeta();
    void LoadLocalCacheServerFooter();
    void LoadPacksDataFromMeta();
    void WaitScanThreadToFinish();
    void StartDelayedRequests();
    // helper functions
    bool ReadLocalFileTableInfoBuffer();
    void FillFileNameIndexes();
    bool SaveServerFooter();
    void DeleteLocalMetaFile() const;
    void ContinueInitialization(float frameDelta);
    bool ReadContentAndExtractFileNames();
    void TestWriteAccessToPackDirectory(const FilePath& dirToDownloadPacks_);
    void TestPackDirectoryExist() const;
    void DumpInitialParams(const FilePath& dirToDownloadPacks, const String& urlToServerSuperpack, const Hints& hints);
    void CreateDownloader();
    void CreateLocalPacks(const String& localPacksDB);

    String BuildErrorMessageBadServerCrc(uint32 crc32) const;
    static String BuildErrorMessageFailedRead(const FilePath& path);
    void UpdateErrorMessageFailedRead();
    static String BuildErrorMessageFailWrite(const FilePath& path);
    void SwapRequestAndUpdatePointers(PackRequest* request, PackRequest* newRequest);
    void SwapPointers(PackRequest* userRequestObject, PackRequest* newRequestObject);
    PackRequest* AddDelayedRequest(const String& requestedPackName);
    void RemoveDownloadedFileIndexes(Vector<uint32>& packIndexes) const;
    void AddRequest(PackRequest* request);
    void RemoveRemoteRequest(PackRequest* request);
    PackRequest* PrepareNewRemoteRequest(const String& requestedPackName);
    PackRequest* CreateNewRemoteRequest(const String& requestedPackName);
    bool IsLocalMetaAndFileTableAlreadyExist() const;
    void TestRetryCountLocalMetaAndGoTo(InitState nextState, InitState alternateState);
    void ClearResouces();
    void OnSettingsChanged(EngineSettings::eSetting value);
    bool IsProfilingEnabled() const;
    String DumpToJsonProfilerTrace();
    static PackRequest* CastToPackRequest(const IRequest* request);

    static uint32 lastCreatedIndexId;

    enum class ScanState : uint32
    {
        Wait,
        Starting,
        Done
    };

    // info to scan local pack files
    struct LocalFileInfo
    {
        String relativeName;
        uint64 sizeOnDevice = std::numeric_limits<uint64>::max();
        uint32 compressedSize = std::numeric_limits<uint32>::max(); // file size can be 0 so use max value default
        uint32 crc32Hash = std::numeric_limits<uint32>::max();
    };
    // fill during scan local pack files, empty after finish scan
    Vector<LocalFileInfo> localFiles;
    // every bit mean file exist and size match with meta
    Vector<bool> scanFileReady;
    Thread* scanThread = nullptr;
    ScanState scanState{ ScanState::Wait };
    Semaphore metaRemoteDataLoadedSem;

    void StartScanDownloadedFiles();
    void ThreadScanFunc();
    void ScanFiles(const FilePath& dir, Vector<LocalFileInfo>& files);
    void RecursiveScan(const FilePath& baseDir, const FilePath& dir, Vector<LocalFileInfo>& files);

    mutable std::ofstream log;

    Engine& engine;
    FilePath localCacheMeta;
    FilePath localCacheFileTable;
    FilePath localCacheFooter;
    FilePath dirToDownloadedPacks;
    String urlToSuperPack;
    bool isProcessingEnabled = false;
    std::unique_ptr<RequestManager> requestManager;
    std::unique_ptr<PackMetaData> metaRemote;
    std::unique_ptr<PackMetaData> metaLocal;

    Vector<PackRequest*> requests; // not forget to delete in destructor
    Vector<PackRequest*> delayedRequests; // move to requests after initialization finished
    Set<std::size_t> requestNameHashes; // for check if request with hash

    List<InitState> skipedStates; // if we can't download from server but have local meta, we can skip some state and use already downloaded meta
    String initErrorMsg;
    InitState initState = InitState::Starting;
    int64 startInitializationTime = 0; // in milliseconds
    bool initTimeoutFired = false;
    std::shared_ptr<MemoryBufferWriter> memBufWriter;
    PackFormat::PackFile::FooterBlock initFooterOnServer; // temp superpack info for every new pack request or during initialization
    PackFormat::PackFile usedPackFile; // current superpack info
    Vector<uint8> buffer; // temp buff
    String uncompressedFileNames;
    UnorderedMap<String, const PackFormat::FileTableEntry*> mapFileData;
    Vector<uint32> startFileNameIndexesInUncompressedNames;
    DLCDownloader::ITask* downloadTask = nullptr;
    uint64 fullSizeServerData = 0;
    mutable Progress lastProgress;

    Hints hints;

    String profilerState;
    DebugGestureListener gestureChecker;

    float32 timeWaitingNextInitializationAttempt = 0;
    uint32 retryCount = 0; // count every initialization error during session

    std::shared_ptr<DLCDownloader> downloader;

    mutable UnorderedSet<uint32> allPacks; // reuse memory

    // collect errno codes and count it, also remember last error code
    size_t errorCounter = 0;
    int32 prevErrorCode = 0;

    bool prevNetworkState = false;
    bool firstTimeNetworkState = false;
};

inline uint32 DLCManagerImpl::GetServerFooterCrc32() const
{
    return initFooterOnServer.infoCrc32;
}

inline const DLCManagerImpl::Hints& DLCManagerImpl::GetHints() const
{
    return hints;
}

inline const PackMetaData& DLCManagerImpl::GetRemoteMeta() const
{
    DVASSERT(metaRemote.get() != nullptr);
    return *metaRemote;
}

inline const PackMetaData& DLCManagerImpl::GetLocalMeta() const
{
    DVASSERT(metaLocal.get() != nullptr);
    return *metaLocal;
}

inline bool DLCManagerImpl::HasLocalMeta() const
{
    return metaLocal.get() != nullptr;
}

inline bool DLCManagerImpl::HasRemoteMeta() const
{
    return metaRemote.get() != nullptr;
}

inline const PackFormat::PackFile& DLCManagerImpl::GetPack() const
{
    return usedPackFile;
}

inline bool DLCManagerImpl::IsFileReady(size_t fileIndex) const
{
    DVASSERT(fileIndex < scanFileReady.size());
    return scanFileReady[fileIndex];
}

inline void DLCManagerImpl::SetFileIsReady(size_t fileIndex, uint32 compressedSize)
{
    scanFileReady[fileIndex] = true;
    lastProgress.alreadyDownloaded += compressedSize;
}

inline bool DLCManagerImpl::IsInQueue(const PackRequest* request) const
{
    DVASSERT(request != nullptr);
    return requestManager->IsInQueue(request->GetRequestedPackName());
}

inline bool DLCManagerImpl::IsTop(const PackRequest* request) const
{
    DVASSERT(request != nullptr);
    if (!requestManager->Empty())
    {
        return request == requestManager->Top();
    }
    return false;
}

} // end namespace DAVA
