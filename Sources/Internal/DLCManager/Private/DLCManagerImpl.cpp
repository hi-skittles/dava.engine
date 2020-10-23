#include "DLCManager/Private/DLCManagerImpl.h"
#include "FileSystem/FileList.h"
#include "FileSystem/File.h"
#include "FileSystem/Private/PackArchive.h"
#include "FileSystem/Private/PackMetaData.h"
#include "FileSystem/FileAPIHelper.h"
#include "DLCManager/DLCDownloader.h"
#include "Utils/CRC32.h"
#include "Logger/Logger.h"
#include "Base/Exception.h"
#include "Time/SystemTimer.h"
#include "Time/DateTime.h"
#include "Engine/Engine.h"
#include "Debug/Private/ImGui.h"
#include "Platform/DeviceInfo.h"
#include "DLCManager/Private/PackRequest.h"
#include "Engine/EngineSettings.h"
#include "Debug/ProfilerCPU.h"
#include "MemoryManager/MemoryProfiler.h"

#include <iomanip>

#if defined(DAVA_MEMORY_PROFILING_ENABLE)
#include <sqlite_modern_cpp.h>
#endif

namespace DAVA
{
DLCManager::~DLCManager() = default;
DLCManager::IRequest::~IRequest() = default;

DLCManager* DLCManager::Create()
{
    using namespace DAVA;
    // just need DAVA::Engine reference
    const EngineContext* context = GetEngineContext();
    if (!context)
    {
        Logger::Error("%s(%d) error: context not created yet.", __FUNCTION__, __LINE__);
        return nullptr;
    }
    DLCManagerImpl* impl = dynamic_cast<DLCManagerImpl*>(context->dlcManager);
    if (!impl)
    {
        Logger::Error("%s(%d) error: no default DLCManager.", __FUNCTION__, __LINE__);
        return nullptr;
    }

    Engine* engine = Engine::Instance();
    if (!engine)
    {
        Logger::Error("%s(%d) error: engine is nullptr.", __FUNCTION__, __LINE__);
        return nullptr;
    }

    return new DLCManagerImpl(engine);
}

void DLCManager::Destroy(DLCManager* dlcManager)
{
    delete dlcManager;
}

const String& DLCManagerImpl::ToString(InitState state)
{
    static const Vector<String> states{
        "Starting",
        "LoadingRequestAskFooter",
        "LoadingRequestGetFooter",
        "LoadingRequestAskFileTable",
        "LoadingRequestGetFileTable",
        "CalculateLocalDBHashAndCompare",
        "LoadingRequestAskMeta",
        "LoadingRequestGetMeta",
        "UnpakingDB",
        "LoadingPacksDataFromLocalMeta",
        "WaitScanThreadToFinish",
        "MoveDeleyedRequestsToQueue",
        "Ready",
        "Offline"
    };
    DVASSERT(states.size() == static_cast<uint32>(InitState::State_COUNT));
    return states.at(static_cast<size_t>(state));
}

static void WriteBufferToFile(const Vector<uint8>& outDB, const FilePath& path)
{
    ScopedPtr<File> f(File::Create(path, File::WRITE | File::CREATE));
    if (!f)
    {
        DAVA_THROW(DAVA::Exception, "can't create file for local DB: " + path.GetStringValue());
    }

    uint32 written = f->Write(outDB.data(), static_cast<uint32>(outDB.size()));
    if (written != outDB.size())
    {
        DAVA_THROW(DAVA::Exception, "can't write file for local DB: " + path.GetStringValue());
    }
}

std::ostream& DLCManagerImpl::GetLog() const
{
    DVASSERT(Thread::IsMainThread());
    return log;
}

DLCDownloader& DLCManagerImpl::GetDownloader() const
{
    if (!downloader)
    {
        DAVA_THROW(Exception, "downloader in nullptr");
    }
    return *downloader;
}

static const std::array<int32, 6> errorForExternalHandle = { { ENAMETOOLONG, ENOSPC, ENODEV,
                                                               EROFS, ENFILE, EMFILE } };

bool DLCManagerImpl::CountError(int32 errCode)
{
    if (errCode != prevErrorCode)
    {
        errorCounter = 0;
        prevErrorCode = errCode;
    }

    size_t yota = 1;

    auto it = std::find(begin(errorForExternalHandle), end(errorForExternalHandle), errCode);
    if (it != end(errorForExternalHandle))
    {
        yota = hints.maxSameErrorCounter;
    }

    errorCounter += yota;

    return errorCounter >= hints.maxSameErrorCounter;
}

bool DLCManagerImpl::IsProfilingEnabled() const
{
    EngineSettings* engineSettings = engine.GetContext()->settings;
    Any value = engineSettings->GetSetting<EngineSettings::SETTING_PROFILE_DLC_MANAGER>();
    return value.Get<bool>(false);
}

uint32 DLCManagerImpl::lastCreatedIndexId = 0;

DLCManagerImpl::DLCManagerImpl(Engine* engine_)
    : instanceIndex(lastCreatedIndexId++)
    , profiler(1024 * 16)
    , engine(*engine_)
{
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    // Configure sqlite memory allocators when memory profiler is enabled
    static bool sqliteHasBeenConfigired = false;
    if (!sqliteHasBeenConfigired)
    {
        sqliteHasBeenConfigired = true;
        sqlite3_mem_methods sqliteMemConfig{};
        sqliteMemConfig.xMalloc = [](int size) -> void* { return MemoryManager::Instance()->Allocate(size, ALLOC_POOL_SQLITE); };
        sqliteMemConfig.xFree = [](void* ptr) { MemoryManager::Instance()->Deallocate(ptr); };
        sqliteMemConfig.xRealloc = [](void* ptr, int size) -> void* { return MemoryManager::Instance()->Reallocate(ptr, size); };
        sqliteMemConfig.xSize = [](void* ptr) -> int { return static_cast<int>(MemoryManager::Instance()->MemorySize(ptr)); };
        sqliteMemConfig.xRoundup = [](int size) -> int { return 0 == size % 8 ? size : size + (8 - (size % 8)); };
        sqliteMemConfig.xInit = [](void*) -> int { return 0; };
        sqliteMemConfig.xShutdown = [](void*) {};
        sqlite3_config(SQLITE_CONFIG_MALLOC, &sqliteMemConfig);
    }
#endif

    DVASSERT(Thread::IsMainThread());
    engine.update.Connect(this, [this](float32 frameDelta)
                          {
                              Update(frameDelta, false);
                          });
    engine.backgroundUpdate.Connect(this, [this](float32 frameDelta)
                                    {
                                        Update(frameDelta, true);
                                    });

    if (IsProfilingEnabled())
    {
        profiler.Start();
    }
    engine.GetContext()->settings->settingChanged.Connect(this, [this](EngineSettings::eSetting value)
                                                          {
                                                              OnSettingsChanged(value);
                                                          });

    gestureChecker.debugGestureMatch.DisconnectAll();

    gestureChecker.debugGestureMatch.Connect(this, [this]() {
        Logger::Debug("enable mini imgui for dlc profiling");
        GetPrimaryWindow()->draw.Disconnect(this);
        GetPrimaryWindow()->draw.Connect(this, [this](Window*) {
            // TODO move it later to common ImGui setting system
            if (ImGui::IsInitialized())
            {
                ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiSetCond_FirstUseEver);
                ImGui::SetNextWindowSize(ImVec2(400, 180), ImGuiSetCond_FirstUseEver);

                bool isImWindowOpened = true;
                String dlcWindowName = "DLC Mng " + std::to_string(instanceIndex);
                ImGui::Begin(dlcWindowName.c_str(), &isImWindowOpened);

                if (isImWindowOpened)
                {
                    if (ImGui::Button("start dlc profiler"))
                    {
                        profiler.Start();
                        profilerState = "started";
                    }

                    if (ImGui::Button("stop dlc profiler"))
                    {
                        profiler.Stop();
                        profilerState = "stopped";
                    }

                    if (ImGui::Button("dump to"))
                    {
                        profilerState = "dumpped: (" + DumpToJsonProfilerTrace() + ")";
                    }

                    ImGui::Text("profiler state: %s", profilerState.c_str());
                }
                else
                {
                    profiler.Stop();
                    GetPrimaryWindow()->draw.Disconnect(this);
                }
                ImGui::End();
            }
        });
    });
}

String DLCManagerImpl::DumpToJsonProfilerTrace()
{
    String outputPath;
    if (profiler.IsStarted() == false)
    {
        FileSystem* fs = GetEngineContext()->fileSystem;
        FilePath docPath = fs->GetPublicDocumentsPath();
        String name = docPath.GetAbsolutePathname() + "dlc_prof.json";
        std::ofstream file(name);
        char buf[16 * 1024];
        file.rdbuf()->pubsetbuf(buf, sizeof(buf));
        if (file)
        {
            Vector<TraceEvent> events = profiler.GetTrace();
            TraceEvent::DumpJSON(events, file);
            outputPath = name;
        }
        else
        {
            outputPath = "cant' open file: " + name;
        }
    }
    else
    {
        outputPath = "error: profiler is started";
    }
    return outputPath;
}

PackRequest* DLCManagerImpl::CastToPackRequest(const IRequest* request)
{
    // PackRequest is the only child - we can use static_cast
    const PackRequest* r = static_cast<const PackRequest*>(request);
    // we can change implementation object as we wish - user not
    PackRequest* packRequest = const_cast<PackRequest*>(r);
    return packRequest;
}

void DLCManagerImpl::OnSettingsChanged(EngineSettings::eSetting value)
{
    if (EngineSettings::SETTING_PROFILE_DLC_MANAGER == value)
    {
        if (IsProfilingEnabled())
        {
            profiler.Start();
        }
        else
        {
            profiler.Stop();
            DumpToJsonProfilerTrace();
        }
    }
}

void DLCManagerImpl::ClearResouces()
{
    if (scanThread)
    {
        scanThread->Cancel();
        metaRemoteDataLoadedSem.Post();
        if (scanThread->IsJoinable())
        {
            scanThread->Join();
        }
        scanThread->Release();
        scanThread = nullptr;
        scanState = ScanState::Wait;
        // reset semaphore in default state.
        // We can't know exactly if metaRemoteDataLoadedSem.Wait() was called in scanThread or not
        // ManualResetEvent but it seem redundant in this case
        metaRemoteDataLoadedSem.~Semaphore();
        new (&metaRemoteDataLoadedSem) Semaphore();
    }
    else
    {
        scanState = ScanState::Wait;
    }

    for (auto request : requests)
    {
        delete request;
    }

    requests.clear();

    for (auto request : delayedRequests)
    {
        delete request;
    }

    initState = InitState::Starting;
    delayedRequests.clear();
    metaRemote.reset();
    metaLocal.reset();
    requestManager.reset();

    buffer.clear();
    uncompressedFileNames.clear();
    mapFileData.clear();
    startFileNameIndexesInUncompressedNames.clear();

    if (downloadTask != nullptr)
    {
        if (downloader != nullptr)
        {
            downloader->RemoveTask(downloadTask);
            downloadTask = nullptr;
        }
    }

    downloader.reset();

    fullSizeServerData = 0;

    timeWaitingNextInitializationAttempt = 0;
    retryCount = 0;

    scanFileReady.clear();

    lastProgress.alreadyDownloaded = 0;
    lastProgress.inQueue = 0;
    lastProgress.isRequestingEnabled = false;
}

DLCManagerImpl::~DLCManagerImpl()
{
    DVASSERT(Thread::IsMainThread());

    engine.update.Disconnect(this);
    engine.backgroundUpdate.Disconnect(this);

    ClearResouces();
}

void DLCManagerImpl::TestWriteAccessToPackDirectory(const FilePath& dirToDownloadPacks_)
{
    const FilePath tmpFile = dirToDownloadPacks_ + "tmp.file";
    {
        ScopedPtr<File> f(File::Create(tmpFile, File::WRITE | File::CREATE));
        if (!f)
        {
            String err = "can't write into directory: " + dirToDownloadedPacks.GetStringValue();
            DAVA_THROW(DAVA::Exception, err);
        }
    }
    FileSystem* fs = GetEngineContext()->fileSystem;
    fs->DeleteFile(tmpFile);
}

void DLCManagerImpl::TestPackDirectoryExist() const
{
    FileSystem* fs = GetEngineContext()->fileSystem;
    if (FileSystem::DIRECTORY_CANT_CREATE == fs->CreateDirectory(dirToDownloadedPacks, true))
    {
        String err = "can't create directory for packs: " + dirToDownloadedPacks.GetStringValue();
        DAVA_THROW(DAVA::Exception, err);
    }
}

static FilePath GetTmpFilePath()
{
    std::stringstream ss;
    DAVA::DateTime dateTime = DAVA::DateTime::Now();
    ss << "~doc:/dlc_manager_log_" << dateTime.GetDay() << '_' << dateTime.GetHour() << '_' << dateTime.GetMinute() << ".log";
    return FilePath(ss.str());
}

void DLCManagerImpl::DumpInitialParams(const FilePath& dirToDownloadPacks, const String& urlToServerSuperpack, const Hints& hints_)
{
    if (!log.is_open())
    {
        FilePath p = hints_.logFilePath.empty() ? GetTmpFilePath() : FilePath(hints_.logFilePath);
        String fullLogPath = p.GetAbsolutePathname();

        log.open(fullLogPath.c_str(), std::ios::trunc);
        if (!log)
        {
            const char* err = strerror(errno);
            Logger::Error("can't create \"%s\" error: %s", fullLogPath.c_str(), err);
            DAVA_THROW(DAVA::Exception, err);
        }

        Logger::Info("DLCManager(%d) log file: %s", instanceIndex, fullLogPath.c_str());

        String preloaded = hints_.preloadedPacks;
        transform(begin(preloaded), end(preloaded), begin(preloaded), [](char c)
                  {
                      return c == '\n' ? ' ' : c;
                  });

        log << "DLCManager(" << instanceIndex << ")::Initialize" << '\n'
            << "(\n"
            << "    dirToDownloadPacks: " << dirToDownloadPacks.GetAbsolutePathname() << '\n'
            << "    urlToServerSuperpack: " << urlToServerSuperpack << '\n'
            << "    hints:\n"
            << "    (\n"
            << "        logFilePath(this file): " << hints_.logFilePath << '\n'
            << "        preloadedPacks: " << preloaded << '\n'
            << "        localPacksDB: " << hints_.localPacksDB << '\n'
            << "        retryConnectMilliseconds: " << hints_.retryConnectMilliseconds << '\n'
            << "        maxFilesToDownload: " << hints_.maxFilesToDownload << '\n'
            << "        timeoutForDownload: " << hints_.timeoutForDownload << '\n'
            << "        skipCDNConnectAfterAttemps: " << hints_.skipCDNConnectAfterAttempts << '\n'
            << "        downloaderMaxHandles: " << hints_.downloaderMaxHandles << '\n'
            << "        downloaderChankBufSize: " << hints_.downloaderChunkBufSize << '\n'
            << "    )\n"
            << ")\n";

        const EnumMap* enumMap = GlobalEnumMap<eGPUFamily>::Instance();
        eGPUFamily e = DeviceInfo::GetGPUFamily();
        const char* gpuFamily = enumMap->ToString(e);
        log << "current_device_gpu: " << gpuFamily << std::endl;
    }
}

void DLCManagerImpl::CreateDownloader()
{
    if (!downloader)
    {
        if (hints.downloader)
        {
            downloader = hints.downloader;
        }
        else
        {
            DLCDownloader::Hints downloaderHints;
            downloaderHints.numOfMaxEasyHandles = static_cast<int>(hints.downloaderMaxHandles);
            downloaderHints.chunkMemBuffSize = static_cast<int>(hints.downloaderChunkBufSize);
            downloaderHints.timeout = static_cast<int>(hints.timeoutForDownload);
            downloaderHints.profiler = &profiler;

            downloader = std::shared_ptr<DLCDownloader>(DLCDownloader::Create(downloaderHints));
        }
    }
}

void DLCManagerImpl::CreateLocalPacks(const String& localPacksDB)
{
    if (!localPacksDB.empty())
    {
        try
        {
            metaLocal = std::make_unique<PackMetaData>(localPacksDB);
        }
        catch (std::exception& ex)
        {
            std::stringstream ss;
            ss << "can't load locat meta data from: " << localPacksDB << "\nerror: " << ex.what();
            log << ss.str() << std::endl;
            Logger::Error("%s", ss.str().c_str());

            DAVA_THROW(Exception, "can't load local meta");
        }
        // add all local packs
        const size_t packsCount = metaLocal->GetPacksCount();
        for (size_t i = 0; i < packsCount; ++i)
        {
            const auto& packInfo = metaLocal->GetPackInfo(static_cast<uint32>(i));
            PackRequest* request = new PackRequest(packInfo.packName);
            AddRequest(request);
        }
    }
}

void DLCManagerImpl::Initialize(const FilePath& dirToDownloadPacks_,
                                const String& urlToServerSuperpack_,
                                const Hints& hints_)
{
    DVASSERT(Thread::IsMainThread());

    profiler.Start();

    const bool isFirstTimeCall = (log.is_open() == false);

    DumpInitialParams(dirToDownloadPacks_, urlToServerSuperpack_, hints_);

    log << __FUNCTION__ << std::endl;

    if (!IsInitialized())
    {
        dirToDownloadedPacks = dirToDownloadPacks_;
        localCacheMeta = dirToDownloadPacks_ + "local_copy_server_meta.meta";
        localCacheFileTable = dirToDownloadPacks_ + "local_copy_server_file_table.block";
        localCacheFooter = dirToDownloadPacks_ + "local_copy_server_footer.footer";
        urlToSuperPack = urlToServerSuperpack_;
        hints = hints_;

        if (!(dirToDownloadedPacks.IsEmpty() && urlToSuperPack.empty()))
        {
            TestPackDirectoryExist();
            TestWriteAccessToPackDirectory(dirToDownloadPacks_);
        }
    }

    CreateDownloader();

    // if Initialize called second time
    fullSizeServerData = 0;
    if (nullptr != downloadTask)
    {
        downloader->RemoveTask(downloadTask);
        downloadTask = nullptr;
    }

    initState = InitState::LoadingRequestAskFooter;

    // safe to call several times, only first will work
    StartScanDownloadedFiles();

    if (isFirstTimeCall)
    {
        CreateLocalPacks(hints_.localPacksDB);
        SetRequestingEnabled(true);
        startInitializationTime = SystemTimer::GetMs();
        if (urlToSuperPack.empty())
        {
            initState = InitState::Ready;
        }
    }
}

void DLCManagerImpl::Deinitialize()
{
    DVASSERT(Thread::IsMainThread());

    log << __FUNCTION__ << std::endl;

    error.DisconnectAll();
    networkReady.DisconnectAll();
    initializeFinished.DisconnectAll();
    requestUpdated.DisconnectAll();
    requestStartLoading.DisconnectAll();

    if (IsInitialized())
    {
        SetRequestingEnabled(false);
    }

    ClearResouces();

    if (profiler.IsStarted())
    {
        profiler.Stop();
        DumpToJsonProfilerTrace();
    }

    log.close();
}

bool DLCManagerImpl::IsInitialized() const
{
    DVASSERT(Thread::IsMainThread());
    return nullptr != requestManager && delayedRequests.empty() && scanThread == nullptr;
}

DLCManagerImpl::InitState DLCManagerImpl::GetInternalInitState() const
{
    DVASSERT(Thread::IsMainThread());
    return initState;
}

const String& DLCManagerImpl::GetLastErrorMessage() const
{
    DVASSERT(Thread::IsMainThread());
    return initErrorMsg;
}

// end Initialization ////////////////////////////////////////

void DLCManagerImpl::Update(float frameDelta, bool inBackground)
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    DVASSERT(Thread::IsMainThread());

    try
    {
        if (InitState::Starting != initState)
        {
            if (initState != InitState::Ready)
            {
                ContinueInitialization(frameDelta);
            }
            else if (isProcessingEnabled)
            {
                if (requestManager)
                {
                    if (hints.fireSignalsInBackground)
                    {
                        inBackground = false;
                    }
                    requestManager->Update(inBackground);
                }
            }
        }
    }
    catch (Exception& ex)
    {
        log << "PackManager error: exception: " << ex.what() << " file: " << ex.file << "(" << ex.line << ")" << std::endl;
        Logger::Error("PackManager error: %s", ex.what());
        throw; // crush or let parent code decide
    }
}

void DLCManagerImpl::WaitScanThreadToFinish()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    if (scanState == ScanState::Done)
    {
        initState = InitState::MoveDeleyedRequestsToQueue;
    }
}

void DLCManagerImpl::ContinueInitialization(float frameDelta)
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    if (timeWaitingNextInitializationAttempt > 0.f)
    {
        timeWaitingNextInitializationAttempt -= frameDelta;
        if (timeWaitingNextInitializationAttempt <= 0.f)
        {
            timeWaitingNextInitializationAttempt = 0.f;
        }
        else
        {
            return;
        }
    }

    if (InitState::Starting == initState)
    {
        initState = InitState::LoadingRequestAskFooter;
    }
    else if (InitState::LoadingRequestAskFooter == initState)
    {
        AskFooter();
    }
    else if (InitState::LoadingRequestGetFooter == initState)
    {
        GetFooter();
    }
    else if (InitState::LoadingRequestAskFileTable == initState)
    {
        AskFileTable();
    }
    else if (InitState::LoadingRequestGetFileTable == initState)
    {
        GetFileTable();
    }
    else if (InitState::CalculateLocalDBHashAndCompare == initState)
    {
        CompareLocalMetaWitnRemoteHash();
    }
    else if (InitState::LoadingRequestAskMeta == initState)
    {
        AskServerMeta();
    }
    else if (InitState::LoadingRequestGetMeta == initState)
    {
        GetServerMeta();
    }
    else if (InitState::UnpakingDB == initState)
    {
        ParseMeta();
    }
    else if (InitState::LoadingPacksDataFromLocalMeta == initState)
    {
        LoadPacksDataFromMeta();
    }
    else if (InitState::WaitScanThreadToFinish == initState)
    {
        WaitScanThreadToFinish();
    }
    else if (InitState::MoveDeleyedRequestsToQueue == initState)
    {
        StartDelayedRequests();
    }
    else if (InitState::Ready == initState)
    {
        // happy end
    }
}

PackRequest* DLCManagerImpl::AddDelayedRequest(const String& requestedPackName)
{
    for (auto* request : delayedRequests)
    {
        if (request->GetRequestedPackName() == requestedPackName)
        {
            return request;
        }
    }

    delayedRequests.push_back(new PackRequest(*this, requestedPackName));
    return delayedRequests.back();
}

void DLCManagerImpl::RemoveDownloadedFileIndexes(Vector<uint32>& packIndexes) const
{
    const auto removeIt = remove_if(begin(packIndexes), end(packIndexes), [&](uint32 index) { return IsFileReady(index); });
    packIndexes.erase(removeIt, end(packIndexes));
}

void DLCManagerImpl::AddRequest(PackRequest* request)
{
    requests.push_back(request);
    if (!request->IsDownloaded())
    {
        requestManager->Push(request);
    }
    requestNameHashes.insert(std::hash<String>{}(request->GetRequestedPackName()));
}

void DLCManagerImpl::RemoveRemoteRequest(PackRequest* request)
{
    const auto it = find(begin(requests), end(requests), request);
    requests.erase(it);
    requestManager->Remove(request);
    requestNameHashes.erase(std::hash<String>{}(request->GetRequestedPackName()));
}
PackRequest* DLCManagerImpl::PrepareNewRemoteRequest(const String& requestedPackName)
{
    Vector<uint32> packIndexes = metaRemote->GetFileIndexes(requestedPackName);

    RemoveDownloadedFileIndexes(packIndexes);

    return new PackRequest(*this, requestedPackName, std::move(packIndexes));
}

PackRequest* DLCManagerImpl::CreateNewRemoteRequest(const String& requestedPackName)
{
    DVASSERT(nullptr == FindRequest(requestedPackName));

    PackRequest* request = PrepareNewRemoteRequest(requestedPackName);

    // we have to do it recursively becouse order of dependency metter
    const Vector<uint32>& depsList = metaRemote->GetPackDependencyIndexes(requestedPackName);

    for (const uint32 dependencyIndex : depsList)
    {
        const PackMetaData::PackInfo& packInfo = metaRemote->GetPackInfo(dependencyIndex);
        const String& depPackName = packInfo.packName;

        if (nullptr == FindRequest(depPackName))
        {
            CreateNewRemoteRequest(depPackName);
        }
    }

    log << "requested: " << requestedPackName << std::endl;

    AddRequest(request);

    return request;
}

bool DLCManagerImpl::IsLocalMetaAndFileTableAlreadyExist() const
{
    FileSystem* fs = engine.GetContext()->fileSystem;
    const bool localFileTableExist = fs->IsFile(localCacheFileTable);
    const bool localMetaExist = fs->IsFile(localCacheMeta);
    return localFileTableExist && localMetaExist;
}

void DLCManagerImpl::TestRetryCountLocalMetaAndGoTo(InitState nextState, InitState alternateState)
{
    ++retryCount;
    timeWaitingNextInitializationAttempt = hints.retryConnectMilliseconds / 1000.f;

    if (initTimeoutFired == false)
    {
        int64 initializationTime = SystemTimer::GetMs() - startInitializationTime;

        if (initializationTime >= (hints.timeoutForInitialization * 1000))
        {
            error.Emit(ErrorOrigin::InitTimeout, EHOSTUNREACH, urlToSuperPack);
            initTimeoutFired = true;
        }
    }

    if (retryCount > hints.skipCDNConnectAfterAttempts)
    {
        if (IsLocalMetaAndFileTableAlreadyExist())
        {
            skipedStates.push_back(initState);
            Logger::Debug("DLCManager skip state from %s to %s, use local meta", ToString(initState).c_str(), ToString(nextState).c_str());
            initState = nextState;
        }
        else
        {
            initState = alternateState;
        }
    }
    else
    {
        initState = alternateState;
    }
}

void DLCManagerImpl::FireNetworkReady(bool nextState)
{
    if (nextState != prevNetworkState || firstTimeNetworkState == false)
    {
        networkReady.Emit(nextState);
        prevNetworkState = nextState;
        firstTimeNetworkState = true;
    }
}

void DLCManagerImpl::AskFooter()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);
    //Logger::FrameworkDebug("pack manager ask_footer");

    DVASSERT(0 == fullSizeServerData);

    if (nullptr == downloadTask)
    {
        downloadTask = downloader->StartGetContentSize(urlToSuperPack);
        if (nullptr == downloadTask)
        {
            DAVA_THROW(Exception, "can't start get_size task with url: " + urlToSuperPack);
        }
    }
    else
    {
        DLCDownloader::TaskStatus status = downloader->GetTaskStatus(downloadTask);

        if (DLCDownloader::TaskState::Finished == status.state)
        {
            downloader->RemoveTask(downloadTask);
            downloadTask = nullptr;

            bool allGood = !status.error.errorHappened;
            if (allGood)
            {
                retryCount = 0;
                fullSizeServerData = status.sizeTotal;
                if (fullSizeServerData < sizeof(PackFormat::PackFile))
                {
                    log << "error: too small superpack on server: " << status << std::endl;
                    DAVA_THROW(DAVA::Exception, "too small superpack on server fullSizeServerData:");
                }
                // start downloading footer from server superpack
                uint64 downloadOffset = fullSizeServerData - sizeof(initFooterOnServer);
                uint32 sizeofFooter = static_cast<uint32>(sizeof(initFooterOnServer));

                memBufWriter.reset(new MemoryBufferWriter(&initFooterOnServer, sizeofFooter));
                downloadTask = downloader->StartTask(urlToSuperPack, memBufWriter, DLCDownloader::Range(downloadOffset, sizeofFooter));
                if (nullptr == downloadTask)
                {
                    DAVA_THROW(Exception, "can't start get_size task with url: " + urlToSuperPack);
                }
                initState = InitState::LoadingRequestGetFooter;
                log << "initState: " << ToString(initState) << std::endl;

                FireNetworkReady(true);
            }
            else
            {
                initErrorMsg = "failed get superpack size on server, download error: ";
                log << initErrorMsg << " " << status << std::endl;

                FireNetworkReady(false);

                TestRetryCountLocalMetaAndGoTo(InitState::LoadingPacksDataFromLocalMeta, InitState::LoadingRequestAskFooter);
            }
        }
    }
}

String DLCManagerImpl::BuildErrorMessageFailWrite(const FilePath& path)
{
    StringStream ss;

    ss << "can't write file: " << path.GetAbsolutePathname() << " errno: (" << errno << ") " << strerror(errno) << '\n';

    // Check available space on device
    const DeviceInfo::StorageInfo* currentDevice = nullptr;
    const List<DeviceInfo::StorageInfo> storageList = DeviceInfo::GetStoragesList();
    for (const DeviceInfo::StorageInfo& info : storageList)
    {
        if (path.StartsWith(info.path))
        {
            if (currentDevice == nullptr)
            {
                currentDevice = &info;
            }
            else if (info.path.GetAbsolutePathname().length() > currentDevice->path.GetAbsolutePathname().length())
            {
                currentDevice = &info;
            }
        }
    }

    if (currentDevice != nullptr)
    {
        ss << "device_type: " << currentDevice->type << '\n'
           << "totalSpace: " << currentDevice->totalSpace << '\n'
           << "freeSpace: " << currentDevice->freeSpace << '\n'
           << "readOnly: " << currentDevice->readOnly << '\n'
           << "removable: " << currentDevice->removable << '\n'
           << "emulated: " << currentDevice->emulated << '\n'
           << "path: " << currentDevice->path.GetStringValue();
    }

    return ss.str();
}

bool DLCManagerImpl::SaveServerFooter()
{
    ScopedPtr<File> f(File::Create(localCacheFooter, File::CREATE | File::WRITE));
    if (f)
    {
        if (sizeof(initFooterOnServer) == f->Write(&initFooterOnServer, sizeof(initFooterOnServer)))
        {
            return true; // all good
        }
    }

    const String errMsg = BuildErrorMessageFailWrite(localCacheFooter);

    if (errMsg != initErrorMsg)
    {
        initErrorMsg = errMsg;
        log << errMsg << std::endl;
        Logger::Error("%s", errMsg.c_str());
        error.Emit(ErrorOrigin::FileIO, errno, localCacheFooter.GetAbsolutePathname());
    }

    return false;
}

String DLCManagerImpl::BuildErrorMessageBadServerCrc(uint32 crc32) const
{
    StringStream ss;
    ss << "error: on server bad superpack!!! Footer not match crc32 "
       << std::hex << crc32 << " != " << std::hex
       << initFooterOnServer.infoCrc32 << std::dec << std::endl;
    return ss.str();
}

void DLCManagerImpl::GetFooter()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    const DLCDownloader::TaskStatus status = downloader->GetTaskStatus(downloadTask);

    if (DLCDownloader::TaskState::Finished == status.state)
    {
        downloader->RemoveTask(downloadTask);
        downloadTask = nullptr;

        const bool allGood = !status.error.errorHappened;
        if (allGood)
        {
            retryCount = 0;
            const uint32 crc32 = CRC32::ForBuffer(reinterpret_cast<char*>(&initFooterOnServer.info), sizeof(initFooterOnServer.info));
            if (crc32 != initFooterOnServer.infoCrc32)
            {
                initErrorMsg = BuildErrorMessageBadServerCrc(crc32);
                log << initErrorMsg;
                Logger::Error("%s", initErrorMsg.c_str());
                TestRetryCountLocalMetaAndGoTo(InitState::LoadingPacksDataFromLocalMeta, InitState::LoadingRequestAskFooter);
                return;
            }

            if (!SaveServerFooter())
            {
                TestRetryCountLocalMetaAndGoTo(InitState::LoadingPacksDataFromLocalMeta, InitState::LoadingRequestAskFooter);
                return;
            }

            usedPackFile.footer = initFooterOnServer;
            initState = InitState::LoadingRequestAskFileTable;
            log << "initState: " << ToString(initState) << std::endl;

            FireNetworkReady(true);
        }
        else
        {
            initErrorMsg = "failed get footer from server, download error: ";
            log << initErrorMsg << " " << status << std::endl;

            FireNetworkReady(false);

            TestRetryCountLocalMetaAndGoTo(InitState::LoadingPacksDataFromLocalMeta, InitState::LoadingRequestAskFooter);
        }
    }
}

void DLCManagerImpl::AskFileTable()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    FileSystem* fs = engine.GetContext()->fileSystem;
    if (fs->IsFile(localCacheFileTable))
    {
        uint32 crc = CRC32::ForFile(localCacheFileTable);
        if (crc == initFooterOnServer.info.filesTableCrc32)
        {
            initState = InitState::LoadingRequestGetFileTable;
            return;
        }
        fs->DeleteFile(localCacheFileTable);
    }

    buffer.resize(initFooterOnServer.info.filesTableSize);

    uint64 downloadOffset = fullSizeServerData - (sizeof(initFooterOnServer) + initFooterOnServer.info.filesTableSize);

    downloadTask = downloader->StartTask(urlToSuperPack, localCacheFileTable.GetAbsolutePathname(), DLCDownloader::Range(downloadOffset, buffer.size()));
    if (nullptr == downloadTask)
    {
        DAVA_THROW(DAVA::Exception, "can't start downloading into buffer");
    }
    initState = InitState::LoadingRequestGetFileTable;
    log << "initState: " << ToString(initState) << std::endl;
}

String DLCManagerImpl::BuildErrorMessageFailedRead(const FilePath& path)
{
    StringStream ss;
    ss << "failed read from file: " << path.GetAbsolutePathname() << " errno(" << errno << ") " << strerror(errno);
    return ss.str();
}

void DLCManagerImpl::UpdateErrorMessageFailedRead()
{
    const String err = BuildErrorMessageFailedRead(localCacheFileTable);
    if (err != initErrorMsg)
    {
        initErrorMsg = err;
        log << initErrorMsg;
    }
}

bool DLCManagerImpl::ReadLocalFileTableInfoBuffer()
{
    uint64 fileSize = 0;
    FileSystem* fs = engine.GetContext()->fileSystem;

    if (!fs->GetFileSize(localCacheFileTable, fileSize))
    {
        UpdateErrorMessageFailedRead();
        return false;
    }

    buffer.resize(static_cast<size_t>(fileSize));

    ScopedPtr<File> f(File::Create(localCacheFileTable, File::OPEN | File::READ));
    if (f)
    {
        const uint32 result = f->Read(buffer.data(), static_cast<uint32>(buffer.size()));
        if (result != buffer.size())
        {
            UpdateErrorMessageFailedRead();
            return false;
        }
    }
    else
    {
        UpdateErrorMessageFailedRead();
        return false;
    }
    return true;
}

void DLCManagerImpl::FillFileNameIndexes()
{
    startFileNameIndexesInUncompressedNames.clear();
    startFileNameIndexesInUncompressedNames.reserve(usedPackFile.filesTable.data.files.size());
    startFileNameIndexesInUncompressedNames.push_back(0); // first name, and skip last '\0' char
    for (uint32 index = 0, last = static_cast<uint32>(uncompressedFileNames.size()) - 1;
         index < last; ++index)
    {
        if (uncompressedFileNames[index] == '\0')
        {
            startFileNameIndexesInUncompressedNames.push_back(index + 1);
        }
    }
}

bool DLCManagerImpl::ReadContentAndExtractFileNames()
{
    if (!ReadLocalFileTableInfoBuffer())
    {
        return false;
    }

    const uint32 crc32 = CRC32::ForBuffer(buffer);
    if (crc32 != initFooterOnServer.info.filesTableCrc32)
    {
        log << "error: FileTable not match crc32" << std::endl;
        return false;
    }

    uncompressedFileNames.clear();
    try
    {
        PackArchive::ExtractFileTableData(initFooterOnServer,
                                          buffer,
                                          uncompressedFileNames,
                                          usedPackFile.filesTable);
    }
    catch (Exception& ex)
    {
        log << ex.file << "(" << ex.line << "): " << ex.what() << std::endl;
        return false;
    }

    FillFileNameIndexes();

    initState = InitState::CalculateLocalDBHashAndCompare;
    log << "initState: " << ToString(initState) << std::endl;

    return true;
}

void DLCManagerImpl::GetFileTable()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    if (downloadTask != nullptr)
    {
        const DLCDownloader::TaskStatus status = downloader->GetTaskStatus(downloadTask);
        if (DLCDownloader::TaskState::Finished == status.state)
        {
            downloader->RemoveTask(downloadTask);
            downloadTask = nullptr;

            const bool allGood = !status.error.errorHappened;
            if (allGood)
            {
                retryCount = 0;
                if (!ReadContentAndExtractFileNames())
                {
                    TestRetryCountLocalMetaAndGoTo(InitState::LoadingPacksDataFromLocalMeta, InitState::LoadingRequestAskFileTable);
                    return;
                }

                FireNetworkReady(true);
            }
            else
            {
                initErrorMsg = "failed get fileTable from server, download error: ";
                log << "error: " << initErrorMsg << std::endl;

                FireNetworkReady(false);

                TestRetryCountLocalMetaAndGoTo(InitState::LoadingPacksDataFromLocalMeta, InitState::LoadingRequestAskFileTable);
            }
        }
    }
    else
    {
        // we already have file without additional request
        if (!ReadContentAndExtractFileNames())
        {
            TestRetryCountLocalMetaAndGoTo(InitState::LoadingPacksDataFromLocalMeta, InitState::LoadingRequestAskFileTable);
        }
    }
}

void DLCManagerImpl::CompareLocalMetaWitnRemoteHash()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    FileSystem* fs = engine.GetContext()->fileSystem;

    if (fs->IsFile(localCacheMeta))
    {
        const uint32 localCrc32 = CRC32::ForFile(localCacheMeta);
        if (localCrc32 != initFooterOnServer.metaDataCrc32)
        {
            DeleteLocalMetaFile();
            // we have to download new localDB file from server!
            initState = InitState::LoadingRequestAskMeta;
        }
        else
        {
            // all good go to
            initState = InitState::LoadingPacksDataFromLocalMeta;
        }
    }
    else
    {
        DeleteLocalMetaFile();

        initState = InitState::LoadingRequestAskMeta;
    }
    log << "initState: " << ToString(initState) << std::endl;
}

void DLCManagerImpl::AskServerMeta()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    uint64 internalDataSize = initFooterOnServer.metaDataSize +
    initFooterOnServer.info.filesTableSize +
    sizeof(PackFormat::PackFile::FooterBlock);

    uint64 downloadOffset = fullSizeServerData - internalDataSize;
    uint64 downloadSize = initFooterOnServer.metaDataSize;

    buffer.resize(static_cast<size_t>(downloadSize));

    memBufWriter.reset(new MemoryBufferWriter(buffer.data(), buffer.size()));

    downloadTask = downloader->StartTask(urlToSuperPack, memBufWriter, DLCDownloader::Range(downloadOffset, downloadSize));
    if (nullptr == downloadTask)
    {
        DAVA_THROW(Exception, "can't start download task into memory buffer");
    }

    initState = InitState::LoadingRequestGetMeta;
    log << "initState: " << ToString(initState) << std::endl;
}

void DLCManagerImpl::GetServerMeta()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    DLCDownloader::TaskStatus status = downloader->GetTaskStatus(downloadTask);

    if (DLCDownloader::TaskState::Finished == status.state)
    {
        downloader->RemoveTask(downloadTask);
        downloadTask = nullptr;

        bool allGood = !status.error.errorHappened;
        if (allGood)
        {
            retryCount = 0;
            initState = InitState::UnpakingDB;
            log << "initState: " << ToString(initState) << std::endl;

            FireNetworkReady(true);
        }
        else
        {
            initErrorMsg = "failed get meta from server, download error: ";
            log << initErrorMsg << status << std::endl;

            FireNetworkReady(false);

            TestRetryCountLocalMetaAndGoTo(InitState::LoadingPacksDataFromLocalMeta, InitState::LoadingRequestAskMeta);
        }
    }
}

void DLCManagerImpl::ParseMeta()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    const uint32 buffCrc32 = CRC32::ForBuffer(buffer);

    try
    {
        if (buffCrc32 != initFooterOnServer.metaDataCrc32)
        {
            log << "on server bad superpack!!! Footer meta not match crc32 "
                << std::hex << buffCrc32 << " != "
                << initFooterOnServer.metaDataCrc32 << std::dec << std::endl;
            DAVA_THROW(Exception, "on server bad superpack!!! Footer meta not match crc32");
        }

        WriteBufferToFile(buffer, localCacheMeta);
    }
    catch (Exception& ex)
    {
        const String errMsg = BuildErrorMessageFailWrite(localCacheMeta);

        if (errMsg != initErrorMsg)
        {
            initErrorMsg = errMsg;
            log << initErrorMsg << std::endl;
            log << "file: " << ex.file << "(" << ex.line << "): " << ex.what() << std::endl;
            error.Emit(ErrorOrigin::FileIO, errno, localCacheMeta.GetAbsolutePathname());
        }
        // lets start all over again
        initState = InitState::LoadingRequestAskFooter;
        return;
    }

    buffer.clear();
    buffer.shrink_to_fit();

    initState = InitState::LoadingPacksDataFromLocalMeta;
    log << "initState: " << ToString(initState) << std::endl;
}

void DLCManagerImpl::LoadLocalCacheServerFooter()
{
    ScopedPtr<File> f(File::Create(localCacheFooter, File::OPEN | File::READ));
    if (f)
    {
        if (sizeof(initFooterOnServer) == f->Read(&initFooterOnServer, sizeof(initFooterOnServer)))
        {
            return;
        }
    }

    log << "can't read file: " << localCacheFooter.GetAbsolutePathname() << " errno(" << errno << ") error: " << strerror(errno) << std::endl;
    DAVA_THROW(Exception, "can't load localCacheFooter data");
}

void DLCManagerImpl::LoadPacksDataFromMeta()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    try
    {
        if (initFooterOnServer.info.packArchiveMarker == Array<char8, 4>{ '\0', '\0', '\0', '\0' })
        {
            // no server data, so use local as is (preload existing file_names_cache)
            LoadLocalCacheServerFooter();

            if (!ReadContentAndExtractFileNames())
            {
                DAVA_THROW(Exception, "can't read and extract FileNamesTable");
            }
        }

        ScopedPtr<File> f(File::Create(localCacheMeta, File::OPEN | File::READ));

        if (!f)
        {
            DAVA_THROW(Exception, "can't open localCacheMeta");
        }

        const uint32 size = static_cast<uint32>(f->GetSize());

        buffer.resize(size);

        const uint32 readSize = f->Read(buffer.data(), size);

        if (size != readSize)
        {
            DAVA_THROW(Exception, "can't read localCacheMeta size not match");
        }

        const uint32 buffHash = CRC32::ForBuffer(buffer);

        if (initFooterOnServer.metaDataCrc32 != buffHash)
        {
            DAVA_THROW(Exception, "can't read localCacheMeta hash not match");
        }

        metaRemote.reset(new PackMetaData(buffer.data(), buffer.size(), uncompressedFileNames));

        const size_t numFiles = metaRemote->GetFileCount();
        scanFileReady.resize(numFiles);

        // now user can do requests for local packs
        requestManager.reset(new RequestManager(*this));
    }
    catch (Exception& ex)
    {
        log << "can't load pack data from meta: " << ex.what() << " file: " << ex.file << "(" << ex.line << ")" << std::endl;
        engine.GetContext()->fileSystem->DeleteFile(localCacheMeta);

        // lets start all over again
        initState = InitState::LoadingRequestAskFooter;
        return;
    }

    metaRemoteDataLoadedSem.Post();

    initState = InitState::WaitScanThreadToFinish;
    log << "initState: " << ToString(initState) << std::endl;
}

void DLCManagerImpl::SwapPointers(PackRequest* userRequestObject, PackRequest* invalidPointer)
{
    auto it = find(begin(requests), end(requests), invalidPointer);
    DVASSERT(it != end(requests));
    // change old pointer (just deleted) to correct one
    *it = userRequestObject;
}

void DLCManagerImpl::SwapRequestAndUpdatePointers(PackRequest* userRequestObject, PackRequest* newRequestObject)
{
    // We want to give user same pointer, so we need move(swap) old object
    // with new object value
    *userRequestObject = *newRequestObject;
    delete newRequestObject; // now this pointer is invalid!
    PackRequest* invalidPointer = newRequestObject;
    // so we have to update all references to new swapped pointer value
    // find new pointer and change it to correct one
    SwapPointers(userRequestObject, invalidPointer);

    requestManager->SwapPointers(userRequestObject, invalidPointer);
}

void DLCManagerImpl::StartDelayedRequests()
{
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);

    if (scanThread != nullptr)
    {
        // scan thread should be finished already
        if (scanThread->IsJoinable())
        {
            scanThread->Join();
        }
        scanThread->Release();
        scanThread = nullptr;
    }

    // I want to create new requests then move its content to old
    // to save pointers for users, if user store pointer to IRequest
    Vector<PackRequest*> tmpRequests;
    delayedRequests.swap(tmpRequests);
    // first remove old request pointers from requestManager
    for (auto request : tmpRequests)
    {
        requestManager->Remove(request);
    }

    for (PackRequest* request : tmpRequests)
    {
        const String& requestedPackName = request->GetRequestedPackName();
        PackRequest* r = FindRequest(requestedPackName);

        if (r == nullptr)
        {
            PackRequest* newRequest = CreateNewRemoteRequest(requestedPackName);
            DVASSERT(newRequest != request);
            DVASSERT(newRequest != nullptr);

            SwapRequestAndUpdatePointers(request, newRequest);
        }
        else
        {
            DVASSERT(r != request);
            // if we come here, it means one of previous requests
            // create it's dependencies and this is it
            SwapRequestAndUpdatePointers(request, r);
        }
    }

    initState = InitState::Ready;

    if (GetInitStatus() == InitStatus::FinishedWithLocalMeta)
    {
        FireNetworkReady(true);
    }

    log << "initState: " << ToString(initState) << std::endl;

    const size_t numDownloaded = count(begin(scanFileReady), end(scanFileReady), true);

    initializeFinished.Emit(numDownloaded, metaRemote->GetFileCount());

    for (PackRequest* request : tmpRequests)
    {
        // we have to inform user because after scanning is finished
        // some request may be already downloaded (all files found)
        requestUpdated.Emit(*request);
    }
}

void DLCManagerImpl::DeleteLocalMetaFile() const
{
    FileSystem* fs = engine.GetContext()->fileSystem;
    fs->DeleteFile(localCacheMeta);
}

bool DLCManagerImpl::IsPackDownloaded(const String& packName) const
{
    DVASSERT(Thread::IsMainThread());

    // packs form local meta data should be already created
    PackRequest* request = FindRequest(packName);
    if (request != nullptr && request->IsDownloaded())
    {
        return true;
    }

    if (!IsInitialized())
    {
        DVASSERT(false && "Initialization not finished. Files are scanning now.");
        log << "Initialization not finished. Files is scanning now." << std::endl;
        return false;
    }

    // check every file in requested pack and all it's dependencies
    const uint32 packIndex = metaRemote->GetPackIndex(packName);
    const Vector<uint32>& deps = metaRemote->GetDependencies(packIndex);

    const auto& allFiles = usedPackFile.filesTable.data.files;
    const size_t size = allFiles.size();

    for (size_t fileIndex = 0; fileIndex < size; ++fileIndex)
    {
        const auto& fileInfo = allFiles[fileIndex];
        if (fileInfo.metaIndex == packIndex || binary_search(begin(deps), end(deps), fileInfo.metaIndex))
        {
            if (!IsFileReady(fileIndex))
            {
                return false;
            }
        }
    }

    return true;
}

DLCManager::InitStatus DLCManagerImpl::GetInitStatus() const
{
    if (!IsInitialized())
    {
        return InitStatus::InProgress;
    }

    if (skipedStates.empty())
    {
        return InitStatus::FinishedWithRemoteMeta;
    }
    return InitStatus::FinishedWithLocalMeta;
}

DLCManager::InitStatus DLCManager::GetInitStatus() const
{
    // default implementation
    return InitStatus::InProgress;
}

uint64 DLCManager::GetPackSize(const String&) const
{
    // default implementation
    return 0;
}

uint64 DLCManagerImpl::GetPackSize(const String& packName) const
{
    uint64 totalSize = 0;
    if (IsInitialized())
    {
        const uint32 packIndex = metaRemote->GetPackIndex(packName);
        const PackMetaData::Dependencies& dependencies = metaRemote->GetDependencies(packIndex);

        const auto& allFiles = usedPackFile.filesTable.data.files;
        for (const auto& fileInfo : allFiles)
        {
            if (fileInfo.metaIndex == packIndex || binary_search(begin(dependencies), end(dependencies), fileInfo.metaIndex))
            {
                totalSize += fileInfo.compressedSize;
            }
        }
    }
    return totalSize;
}

const DLCManager::IRequest* DLCManagerImpl::RequestPack(const String& packName)
{
    DVASSERT(Thread::IsMainThread());

    const PackRequest* request = FindRequest(packName);
    if (request != nullptr)
    {
        return request;
    }

    if (!IsInitialized())
    {
        request = AddDelayedRequest(packName);
        return request;
    }

    request = CreateNewRemoteRequest(packName);
    return request;
}

void DLCManagerImpl::SetRequestPriority(const IRequest* request)
{
    DVASSERT(Thread::IsMainThread());

    if (request != nullptr)
    {
        log << "set_request_priority: " << request->GetRequestedPackName() << std::endl;

        PackRequest* req = CastToPackRequest(request);

        if (IsInitialized())
        {
            requestManager->SetPriorityToRequest(req);
        }
        else
        {
            const auto it = std::find(begin(delayedRequests), end(delayedRequests), request);
            if (it != end(delayedRequests))
            {
                delayedRequests.erase(it);
                delayedRequests.insert(delayedRequests.begin(), req);
            }
        }
    }
}

void DLCManagerImpl::RemovePack(const String& requestedPackName)
{
    DVASSERT(Thread::IsMainThread());

    // now we can work without CDN, so always wait for initialization is done
    if (!IsInitialized())
    {
        log << "error: can't remove pack: " << requestedPackName << " initialization not finished yet." << std::endl;
        return;
    }

    if (HasLocalMeta() && GetLocalMeta().HasPack(requestedPackName))
    {
        log << "error: pack " << requestedPackName << " is local and can't be removed" << std::endl;
        DVASSERT(false);
        return;
    }

    const IRequest* request = RequestPack(requestedPackName);
    if (request != nullptr)
    {
        PackRequest* packRequest = CastToPackRequest(request);

        // check requestedPackName itself is not dependency from other pack in queue
        // WARNING you may delete packs ONLY without external dependency
        auto CheckOtherParentPackInQueue = [&]() {
            if (!metaRemote)
            {
                return false;
            }
            const Vector<PackRequest*>& inQueueRequests = requestManager->GetRequests();

            auto IsParentDependency = [&](const PackRequest* parentPack) {
                const String& parentPackName = parentPack->GetRequestedPackName();
                uint32 parentIndex = metaRemote->GetPackIndex(parentPackName);
                uint32 childIndex = metaRemote->GetPackIndex(requestedPackName);
                if (parentIndex != childIndex && metaRemote->HasDependency(parentIndex, childIndex))
                {
                    log << "error: try remove pack: " << requestedPackName << " but this pack is dependency from: " << parentPackName << " currently in request queue." << std::endl;
                    return true;
                }
                return false;
            };

            bool parentDependencyExists = std::any_of(begin(inQueueRequests), end(inQueueRequests), IsParentDependency);
            return parentDependencyExists;
        };

        DVASSERT(CheckOtherParentPackInQueue() == false); // only for debug

        const Vector<uint32>& directDependencies = packRequest->GetDirectDependencies();

        requestManager->Remove(packRequest);

        const auto it = find(begin(requests), end(requests), request);
        if (it != end(requests))
        {
            requests.erase(it);
        }

        log << "removing: " << requestedPackName << std::endl;

        delete request;

        if (metaRemote)
        {
            StringStream undeletedFiles;
            FileSystem* fs = GetEngineContext()->fileSystem;
            // remove all files for pack
            Vector<uint32> fileIndexes = metaRemote->GetFileIndexes(requestedPackName);
            for (uint32 index : fileIndexes)
            {
                if (IsFileReady(index))
                {
                    const String relFile = GetRelativeFilePath(index);

                    FilePath filePath = dirToDownloadedPacks + (relFile + extDvpl);
                    if (!fs->DeleteFile(filePath))
                    {
                        if (fs->IsFile(filePath))
                        {
                            undeletedFiles << filePath.GetStringValue() << '\n';
                        }
                    }
                    scanFileReady[index] = false; // clear flag anyway
                }
            }
            String errMsg = undeletedFiles.str();
            if (!errMsg.empty())
            {
                log << "can't delete files: " << errMsg << std::endl;
                Logger::Error("can't delete files: %s", errMsg.c_str());
            }
        }

        // now remove dependencies
        // we have to remove packs in reverse order for debug purpeses see: CheckOtherParentPackInQueue
        for (uint32 dependent : directDependencies)
        {
            const String& depPackName = metaRemote->GetPackInfo(dependent).packName;
            PackRequest* depRequest = FindRequest(depPackName);
            if (nullptr != depRequest)
            {
                // make copy name to prevent UB, after deleting pack
                const String packToRemove = depRequest->GetRequestedPackName();
                RemovePack(packToRemove);
            }
        }
    }
    else
    {
        log << "error: can't remove not found pack: " << requestedPackName << std::endl;
    }
}

void DLCManager::ResetQueue()
{
}

void DLCManagerImpl::ResetQueue()
{
    DVASSERT(Thread::IsMainThread());

    if (IsInitialized() && requestManager)
    {
        log << "reset_queue" << std::endl;
        // do NOT use reference
        const Vector<PackRequest*> requests = requestManager->GetRequests();

        requestManager->Clear();

        for (PackRequest* r : requests)
        {
            RemoveRemoteRequest(r);
            delete r;
        }
    }
    else
    {
        log << "error: can't reset_queue - initialization not finished yet.";
    }
}

DLCManager::Progress DLCManagerImpl::GetProgress() const
{
    using namespace DAVA;
    using namespace PackFormat;

    DVASSERT(Thread::IsMainThread());

    if (!IsInitialized())
    {
        lastProgress.isRequestingEnabled = false;
        return lastProgress;
    }

    // count total only once
    if (lastProgress.total == 0)
    {
        const Vector<PackFile::FilesTableBlock::FilesData::Data>& files = usedPackFile.filesTable.data.files;
        for (const auto& fileData : files)
        {
            lastProgress.total += fileData.compressedSize;
        }
    }

    {
        lastProgress.alreadyDownloaded = 0;
        lastProgress.inQueue = 0;
        const Vector<PackFile::FilesTableBlock::FilesData::Data>& files = usedPackFile.filesTable.data.files;
        const size_t numFiles = files.size();
        for (size_t fileIndex = 0; fileIndex < numFiles; ++fileIndex)
        {
            const auto& fileData = files[fileIndex];
            if (IsFileReady(fileIndex))
            {
                lastProgress.alreadyDownloaded += fileData.compressedSize;
            }
            else
            {
                const PackMetaData::PackInfo& packInfo = metaRemote->GetPackInfo(fileData.metaIndex);
                if (requestManager->IsInQueue(packInfo.packName))
                {
                    lastProgress.inQueue += fileData.compressedSize;
                }
            }
        }
    }

    lastProgress.isRequestingEnabled = IsRequestingEnabled();

    return lastProgress;
}

DLCManager::Progress DLCManager::GetPacksProgress(const Vector<String>& packNames) const
{
    return Progress();
}

DLCManager::Progress DLCManagerImpl::GetPacksProgress(const Vector<String>& packNames) const
{
    using namespace DAVA;
    DVASSERT(Thread::IsMainThread());

    if (!IsInitialized())
    {
        return Progress();
    }

    // 1. make flat set with all pack with it's dependencies
    // 2. go throw all files and check if it's pack in set

    allPacks.clear();

    const size_t packsCount = metaRemote->GetPacksCount();

    allPacks.reserve(packsCount);

    for (const String& packName : packNames)
    {
        uint32 packIndex = metaRemote->GetPackIndex(packName);
        const Vector<uint32>& childrenPacks = metaRemote->GetDependencies(packIndex);
        for (const uint32 childPackIndex : childrenPacks)
        {
            allPacks.insert(childPackIndex);
        }
        allPacks.insert(packIndex);
    }

    Progress result;
    result.isRequestingEnabled = IsRequestingEnabled();
    // go throw all files
    const auto& allFiles = usedPackFile.filesTable.data.files;
    const size_t numFiles = allFiles.size();
    for (size_t fileIndex = 0; fileIndex < numFiles; ++fileIndex)
    {
        const auto& fileInfo = allFiles[fileIndex];
        if (allPacks.find(fileInfo.metaIndex) != end(allPacks))
        {
            result.total += fileInfo.compressedSize;
            if (IsFileReady(fileIndex))
            {
                result.alreadyDownloaded += fileInfo.compressedSize;
            }
        }
    }

    return result;
}

DLCManager::Info DLCManager::GetInfo() const
{
    return Info{};
}

DLCManager::Info DLCManagerImpl::GetInfo() const
{
    Info info;
    if (IsInitialized())
    {
        info.infoCrc32 = initFooterOnServer.infoCrc32;
        info.metaCrc32 = initFooterOnServer.metaDataCrc32;
        info.totalFiles = static_cast<uint32>(metaRemote->GetFileCount());
    }
    return info;
}

DLCManager::FileInfo DLCManager::GetFileInfo(const FilePath& /*path*/) const
{
    return FileInfo{};
}

DLCManager::FileInfo DLCManagerImpl::GetFileInfo(const FilePath& path) const
{
    FileInfo fileInfo;

    if (!IsInitialized())
    {
        return fileInfo;
    }

    if (HasLocalMeta())
    {
        // no information for local(static files in APK) files
        const PackMetaData& meta = GetLocalMeta();
        const FileNamesTree& tree = meta.GetFileNamesTree();
        if (tree.Find(fileInfo.relativePathInMeta))
        {
            fileInfo.isLocalFile = true;
            fileInfo.isKnownFile = true;
        }
    }

    if (HasRemoteMeta())
    {
        const PackMetaData& meta = GetRemoteMeta();
        const FileNamesTree& remoteFilesTree = meta.GetFileNamesTree();

        fileInfo.relativePathInMeta = path.StartsWith("~res:/") ? path.GetRelativePathname("~res:/") : path.GetRelativePathname();

        if (remoteFilesTree.Find(fileInfo.relativePathInMeta))
        {
            fileInfo.isRemoteFile = true;
            fileInfo.isKnownFile = true;

            const auto it = mapFileData.find(fileInfo.relativePathInMeta);
            if (it != end(mapFileData))
            {
                const PackFormat::FileTableEntry* entry = it->second;

                fileInfo.indexOfPackInMeta = entry->metaIndex;
                const PackMetaData::PackInfo& packInfo = meta.GetPackInfo(entry->metaIndex);
                fileInfo.packName = packInfo.packName;

                const size_t indexInString = uncompressedFileNames.find(fileInfo.relativePathInMeta);
                DVASSERT(indexInString != String::npos);
                const String& str = uncompressedFileNames;
                const size_t numOfNullChars = std::count(begin(str), begin(str) + indexInString + 1, '\0');
                const uint32 fileIndex = static_cast<uint32>(numOfNullChars); // will match index of string and index of file in filesTable

                fileInfo.indexOfFileInMeta = fileIndex;
                fileInfo.hashCompressedInMeta = entry->compressedCrc32;
                fileInfo.hashUncompressedInMeta = entry->originalCrc32;
                fileInfo.sizeCompressedInMeta = entry->compressedSize;
                fileInfo.sizeUncompressedInMeta = entry->originalSize;
                fileInfo.isDlcMngThinkFileReady = IsFileReady(fileIndex);
            }
        }
    }
    return fileInfo;
}

bool DLCManagerImpl::IsRequestingEnabled() const
{
    DVASSERT(Thread::IsMainThread());
    return isProcessingEnabled;
}

void DLCManagerImpl::SetRequestingEnabled(bool value)
{
    DVASSERT(Thread::IsMainThread());

    log << "requesting_enabled: " << std::boolalpha << value << std::noboolalpha << std::endl;

    if (value)
    {
        if (!isProcessingEnabled)
        {
            isProcessingEnabled = true;
            if (requestManager)
            {
                requestManager->Start();
            }
        }
    }
    else
    {
        if (isProcessingEnabled)
        {
            isProcessingEnabled = false;
            if (requestManager)
            {
                requestManager->Stop();
            }
        }
    }
}

PackRequest* DLCManagerImpl::FindRequest(const String& requestedPackName) const
{
    DVASSERT(Thread::IsMainThread());

    // optimization. Fast return nullptr if no such request
    const std::size_t hash = std::hash<String>{}(requestedPackName);
    if (0 == requestNameHashes.count(hash))
    {
        return nullptr;
    }

    for (auto request : requests)
    {
        if (request->GetRequestedPackName() == requestedPackName)
        {
            return request;
        }
    }

    for (auto request : delayedRequests)
    {
        if (request->GetRequestedPackName() == requestedPackName)
        {
            return request;
        }
    }

    return nullptr;
}

bool DLCManager::IsAnyPackInQueue() const
{
    return false;
}

bool DLCManagerImpl::IsAnyPackInQueue() const
{
    if (IsInitialized())
    {
        return !requestManager->Empty();
    }
    return false;
}

bool DLCManager::IsKnownFile(const FilePath&) const
{
    return false;
}

bool DLCManagerImpl::IsKnownFile(const FilePath& path) const
{
    String rel;
    if (path.StartsWith("~res:/"))
    {
        rel = path.GetRelativePathname("~res:/");
    }
    else
    {
        rel = path.GetRelativePathname();
    }

    if (HasLocalMeta())
    {
        const auto& meta = GetLocalMeta();
        const auto& tree = meta.GetFileNamesTree();
        if (tree.Find(rel))
        {
            return true;
        }
    }
    if (HasRemoteMeta())
    {
        const auto& meta = GetRemoteMeta();
        const auto& remoteFilesTree = meta.GetFileNamesTree();
        if (remoteFilesTree.Find(rel))
        {
            return true;
        }
    }
    return false;
}

bool DLCManagerImpl::IsPackInQueue(const String& packName) const
{
    DVASSERT(Thread::IsMainThread());
    if (!IsInitialized())
    {
        return false;
    }

    return requestManager->IsInQueue(packName);
}

const FilePath& DLCManagerImpl::GetLocalPacksDirectory() const
{
    DVASSERT(Thread::IsMainThread());
    return dirToDownloadedPacks;
}

const String& DLCManagerImpl::GetSuperPackUrl() const
{
    DVASSERT(Thread::IsMainThread());
    return urlToSuperPack;
}

String DLCManagerImpl::GetRelativeFilePath(uint32 fileIndex)
{
    const uint32 startOfFilePath = startFileNameIndexesInUncompressedNames.at(fileIndex);
    return &uncompressedFileNames.at(startOfFilePath);
}

void DLCManagerImpl::StartScanDownloadedFiles()
{
    if (!urlToSuperPack.empty())
    {
        if (ScanState::Wait == scanState)
        {
            scanState = ScanState::Starting;
            scanThread = Thread::Create(MakeFunction(this, &DLCManagerImpl::ThreadScanFunc));
            String name = String("DLC(") + std::to_string(instanceIndex) + ")::ThreadScan";
            scanThread->SetName(name);
            scanThread->Start();
        }
    }
}

void DLCManagerImpl::RecursiveScan(const FilePath& baseDir, const FilePath& dir, Vector<LocalFileInfo>& files)
{
    ScopedPtr<FileList> fl(new FileList(dir, false));

    for (uint32 index = 0; index < fl->GetCount(); ++index)
    {
        const FilePath& path = fl->GetPathname(index);
        if (fl->IsNavigationDirectory(index))
        {
            continue;
        }
        if (fl->IsDirectory(index))
        {
            RecursiveScan(baseDir, path, files);
        }
        else
        {
            if (path.GetExtension() == extDvpl)
            {
                String fileName = path.GetAbsolutePathname();

                FILE* f = FileAPI::OpenFile(fileName, "rb");
                if (f == nullptr)
                {
                    Logger::Info("can't open file %s during scan", fileName.c_str());
                    continue;
                }

                bool needDeleteIncompleteFile = false;
                int32 footerSize = sizeof(PackFormat::LitePack::Footer);
                if (0 == fseek(f, -footerSize, SEEK_END))
                {
                    PackFormat::LitePack::Footer footer;
                    if (footerSize == fread(&footer, 1, footerSize, f))
                    {
                        LocalFileInfo info;
                        info.sizeOnDevice = FileAPI::GetFileSize(fileName);
                        info.relativeName = path.GetRelativePathname(baseDir);
                        info.compressedSize = footer.sizeCompressed;
                        info.crc32Hash = footer.crc32Compressed;
                        files.push_back(info);
                    }
                    else
                    {
                        needDeleteIncompleteFile = true;
                        Logger::Info("can't read footer in file: %s", fileName.c_str());
                    }
                }
                else
                {
                    needDeleteIncompleteFile = true;
                    Logger::Info("can't seek to dvpl footer in file: %s", fileName.c_str());
                }
                FileAPI::Close(f);
                if (needDeleteIncompleteFile)
                {
                    if (0 != FileAPI::RemoveFile(fileName))
                    {
                        Logger::Error("can't delete incomplete file: %s", fileName.c_str());
                    }
                }
            }
        }
    }
}

void DLCManagerImpl::ScanFiles(const FilePath& dir, Vector<LocalFileInfo>& files)
{
    if (FileSystem::Instance()->IsDirectory(dir))
    {
        files.clear();
        files.reserve(hints.maxFilesToDownload);
        RecursiveScan(dir, dir, files);
    }
}

void DLCManagerImpl::ThreadScanFunc()
{
    Thread* thisThread = Thread::Current();
    // scan files in download dir
    const int64 startTime = SystemTimer::GetMs();

    ScanFiles(dirToDownloadedPacks, localFiles);

    const int64 finishScan = SystemTimer::GetMs() - startTime;

    Logger::Info("finish scan files for: %fsec total files: %ld", finishScan / 1000.f, localFiles.size());

    if (thisThread->IsCancelling())
    {
        return;
    }

    metaRemoteDataLoadedSem.Wait();

    if (thisThread->IsCancelling() || metaRemote == nullptr)
    {
        if (metaRemote == nullptr)
        {
            Logger::Error("remote meta not loaded");
        }
        return;
    }

    // merge with meta
    // Yes! is pack loaded before meta
    const PackFormat::PackFile& pack = GetPack();

    Vector<ResourceArchive::FileInfo> filesInfo;
    PackArchive::FillFilesInfo(pack, uncompressedFileNames, mapFileData, filesInfo);

    String relativeNameWithoutDvpl;

    if (thisThread->IsCancelling())
    {
        return;
    }

    FileSystem* fs = GetEngineContext()->fileSystem;

    for (const LocalFileInfo& info : localFiles)
    {
        relativeNameWithoutDvpl = info.relativeName.substr(0, info.relativeName.size() - 5);
        const PackFormat::FileTableEntry* entry = mapFileData[relativeNameWithoutDvpl];
        if (entry != nullptr)
        {
            if (entry->compressedCrc32 == info.crc32Hash &&
                entry->compressedSize == info.compressedSize &&
                entry->compressedSize + sizeof(PackFormat::LitePack::Footer) == info.sizeOnDevice)
            {
                size_t fileIndex = std::distance(&pack.filesTable.data.files[0], entry);
                SetFileIsReady(fileIndex, info.compressedSize);
            }
            else
            {
                // need to continue downloading file
                // leave it as is
            }
        }
        else
        {
            // no such file on server, delete it
            fs->DeleteFile(dirToDownloadedPacks + info.relativeName);
        }
    }

    if (thisThread->IsCancelling())
    {
        return;
    }

    DAVA::RunOnMainThreadAsync([this]()
                               {
                                   // finish thread
                                   scanState = ScanState::Done;
                               });
}

} // end namespace DAVA
