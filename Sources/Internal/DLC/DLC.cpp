#include "DLC.h"

#include "Downloader/DownloadManager.h"
#include "Engine/Engine.h"
#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"
#include "Job/JobManager.h"
#include "Logger/Logger.h"
#include "Patcher/PatchFile.h"
#include "Platform/DeviceInfo.h"
#include "Render/GPUFamilyDescriptor.h"
#include "Utils/StringFormat.h"



#include <functional>

namespace DAVA
{
DLC::DLC(const String& url, const FilePath& sourceDir, const FilePath& destinationDir, const FilePath& workingDir, const String& gameVersion, const FilePath& resVersionPath, bool forceFullUpdate)
    : dlcState(DS_INIT)
    , dlcError(DE_NO_ERROR)
    , patchingThread(NULL)
{
    DVASSERT(workingDir.IsDirectoryPathname());
    DVASSERT(FileSystem::Instance()->Exists(workingDir));

    DVASSERT(destinationDir.IsDirectoryPathname());
    DVASSERT(FileSystem::Instance()->Exists(destinationDir));

    DVASSERT(sourceDir.IsDirectoryPathname());
    DVASSERT(FileSystem::Instance()->Exists(sourceDir));

    DVASSERT(!gameVersion.empty());

    //  we suppose that downloaded data should not be media data and exclude it from index.
    FileSystem::Instance()->MarkFolderAsNoMedia(destinationDir);

    // initial values
    dlcContext.remoteUrl = url;
    dlcContext.gameVer = gameVersion;
    dlcContext.localVer = 0;
    dlcContext.localGameVer = "";
    dlcContext.forceFullUpdate = forceFullUpdate;

    dlcContext.localWorkingDir = workingDir;
    dlcContext.localDestinationDir = destinationDir;
    dlcContext.localSourceDir = sourceDir;
    dlcContext.localVerStorePath = resVersionPath;

    dlcContext.remoteVerUrl = Format("%s/g%s.info", url.c_str(), gameVersion.c_str());
    dlcContext.remoteVer = 0;
    dlcContext.remoteVerDownloadId = 0;
    dlcContext.remoteFullSizeDownloadId = 0;
    dlcContext.remoteLiteSizeDownloadId = 0;
    dlcContext.remoteMetaDownloadId = 0;
    dlcContext.remoteVerStotePath = workingDir + "RemoteVersion.info";
    dlcContext.remotePatchDownloadId = 0;
    dlcContext.remotePatchSize = 0;
    dlcContext.remotePatchReadySize = 0;
    dlcContext.remotePatchStorePath = workingDir + "Remote.patch";
    dlcContext.remoteMetaStorePath = workingDir + "Remote.meta";

    dlcContext.patchInProgress = true;
    dlcContext.totalPatchCount = 0;
    dlcContext.appliedPatchCount = 0;
    dlcContext.patchingError = PatchFileReader::ERROR_NO;
    dlcContext.lastErrno = 0;
    dlcContext.lastPatchingErrorDetails = PatchFileReader::PatchingErrorDetails();

    dlcContext.downloadInfoStorePath = workingDir + "Download.info";
    dlcContext.stateInfoStorePath = workingDir + "State.info";
    dlcContext.prevState = 0;

    ReadValue(dlcContext.stateInfoStorePath, &dlcContext.prevState);
    ReadValue(dlcContext.remoteVerStotePath, &dlcContext.remoteVer);

    logsFilePath = workingDir + "DlcLog.txt";

    // FSM variables
    fsmAutoReady = false;

    DownloadManager* dm = DownloadManager::Instance();
    dm->downloadTaskStateChanged.Connect(this, &DLC::OnDownloadTaskStateChanged);
}

DLC::~DLC()
{
    DownloadManager* dm = DownloadManager::Instance();
    dm->downloadTaskStateChanged.Disconnect(this);

    DVASSERT((dlcState == DS_INIT || dlcState == DS_READY || dlcState == DS_DONE) && "DLC can be safely destroyed only in certain modes");
}

void DLC::Check()
{
    PostEvent(EVENT_CHECK_START);
}

void DLC::Start()
{
    PostEvent(EVENT_DOWNLOAD_START);
}

void DLC::Cancel()
{
    PostEvent(EVENT_CANCEL);
}

void DLC::GetProgress(uint64& cur, uint64& total) const
{
    switch (dlcState)
    {
    case DS_READY:
        total = dlcContext.remotePatchSize;
        cur = dlcContext.remotePatchReadySize;
        break;
    case DS_DOWNLOADING:
        total = dlcContext.remotePatchSize;
        DownloadManager::Instance()->GetProgress(dlcContext.remotePatchDownloadId, cur);
        break;
    case DS_PATCHING:
        total = dlcContext.totalPatchCount;
        cur = dlcContext.appliedPatchCount;
        break;
    default:
        cur = 0;
        total = 0;
        break;
    }
}

DLC::DLCState DLC::GetState() const
{
    return dlcState;
}

DLC::DLCError DLC::GetError() const
{
    return dlcError;
}

int32 DLC::GetLastErrno() const
{
    return dlcContext.lastErrno;
}

PatchFileReader::PatchingErrorDetails DLC::GetLastErrorInfo() const
{
    return dlcContext.lastPatchingErrorDetails;
}

PatchFileReader::PatchError DLC::GetPatchError() const
{
    return dlcContext.patchingError;
}

FilePath DLC::GetMetaStorePath() const
{
    return dlcContext.remoteMetaStorePath;
}

void DLC::SetLogFileName(const FilePath& customLogFileName)
{
    logsFilePath = customLogFileName;
}

void DLC::PostEvent(DLCEvent event)
{
    Function<void()> fn = Bind(&DLC::FSM, this, event);
    GetEngineContext()->jobManager->CreateMainJob(fn);
}

void DLC::PostError(DLCError error)
{
    dlcError = error;
    PostEvent(EVENT_ERROR);
}

void DLC::FSM(DLCEvent event)
{
    bool eventHandled = true;
    DLCState oldState = dlcState;

    // State Machine implementation
    // Read more - http://www.ict.edu.ru/ft/001845/switch.pdf

    switch (dlcState)
    {
    case DS_INIT:
        switch (event)
        {
        case EVENT_DOWNLOAD_START:
            fsmAutoReady = true;
        // don't break here

        case EVENT_CHECK_START:
            // if last time stopped on the patching state and patch file exists - continue patching
            if (!dlcContext.forceFullUpdate &&
                DS_PATCHING == dlcContext.prevState &&
                FileSystem::Instance()->Exists(dlcContext.remotePatchStorePath) &&
                FileSystem::Instance()->Exists(dlcContext.remoteVerStotePath))
            {
                dlcContext.prevState = 0;
                dlcState = DS_PATCHING;
            }
            else
            {
                dlcState = DS_CHECKING_INFO;
            }
            break;
        case EVENT_CANCEL:
            dlcError = DE_WAS_CANCELED;
            dlcState = DS_DONE;
            break;
        default:
            eventHandled = false;
            break;
        }
        break;

    case DS_CHECKING_INFO:
        switch (event)
        {
        case EVENT_CHECK_OK:
            // if remote version is newer than local we should continue DLC
            if (dlcContext.localVer < dlcContext.remoteVer)
            {
                // check if patch exists on server
                dlcState = DS_CHECKING_PATCH;
            }
            else
            {
                // local version is up do date. finish DLC
                dlcState = DS_CLEANING;
            }
            break;
        case EVENT_ERROR:
            dlcState = DS_DONE;
            break;
        case EVENT_CANCEL:
            dlcState = DS_CANCELLING;
            break;
        default:
            eventHandled = false;
            break;
        }
        break;

    case DS_CHECKING_PATCH:
        switch (event)
        {
        case EVENT_CHECK_OK:
            dlcState = DS_CHECKING_META;
            break;
        case EVENT_ERROR:
            dlcState = DS_DONE;
            break;
        case EVENT_CANCEL:
            dlcState = DS_CANCELLING;
            break;
        default:
            eventHandled = false;
            break;
        }
        break;

    case DS_CHECKING_META:
        switch (event)
        {
        case EVENT_CHECK_OK:
            // automatically start download after check?
            if (fsmAutoReady)
            {
                // download patch
                dlcState = DS_DOWNLOADING;
            }
            else
            {
                dlcState = DS_READY;
            }
            break;
        case EVENT_ERROR:
            dlcState = DS_DONE;
            break;
        case EVENT_CANCEL:
            dlcState = DS_CANCELLING;
            break;
        default:
            eventHandled = false;
            break;
        }
        break;

    case DS_READY:
        switch (event)
        {
        case EVENT_DOWNLOAD_START:
            dlcState = DS_DOWNLOADING;
            break;
        case EVENT_CANCEL:
            dlcError = DE_WAS_CANCELED;
            dlcState = DS_DONE;
            break;
        default:
            eventHandled = false;
            break;
        }
        break;

    case DS_DOWNLOADING:
        switch (event)
        {
        case EVENT_DOWNLOAD_OK:
            dlcState = DS_PATCHING;
            break;
        case EVENT_ERROR:
            dlcState = DS_DONE;
            break;
        case EVENT_CANCEL:
            dlcState = DS_CANCELLING;
            break;
        default:
            eventHandled = false;
            break;
        }
        break;
    case DS_PATCHING:
        switch (event)
        {
        case EVENT_PATCH_OK:
            // a new version can appear while we doing patching
            // so we need to check it once again
            fsmAutoReady = true;
            dlcState = DS_CHECKING_INFO;
            break;
        case EVENT_ERROR:
            dlcState = DS_DONE;
            break;
        case EVENT_CANCEL:
            dlcState = DS_CANCELLING;
            break;
        default:
            eventHandled = false;
            break;
        }
        break;
    case DS_CANCELLING:
        switch (event)
        {
        case EVENT_CHECK_OK:
        case EVENT_DOWNLOAD_OK:
        case EVENT_PATCH_OK:
        case EVENT_ERROR:
            dlcError = DE_WAS_CANCELED;
            dlcState = DS_DONE;
            break;
        default:
            eventHandled = false;
            break;
        }
        break;
    case DS_CLEANING:
        switch (event)
        {
        case EVENT_CLEAN_OK:
            dlcState = DS_DONE;
            break;
        case EVENT_CANCEL:
            break;
        default:
            eventHandled = false;
            break;
        }
        break;
    case DS_DONE:
        break;
    default:
        break;
    }

    if (EVENT_ERROR == event)
    {
        DVASSERT(DE_NO_ERROR != dlcError && "Unhandled error, dlcError is set to NO_ERROR");
    }

    if (!eventHandled)
    {
        Logger::ErrorToFile(logsFilePath, "[DLC::FSM] Unhanded event %d in state %d\n", event, dlcState);
        DVASSERT(false);
    }

    // what should we do, when state changed
    if (oldState != dlcState)
    {
        Logger::InfoToFile(logsFilePath, "[DLC::FSM] Changing state %d->%d", oldState, dlcState);

        switch (dlcState)
        {
        case DS_INIT:
            break;
        case DS_CHECKING_INFO:
            StepCheckInfoBegin();
            break;
        case DS_CHECKING_PATCH:
            StepCheckPatchBegin();
            break;
        case DS_CHECKING_META:
            StepCheckMetaBegin();
            break;
        case DS_READY:
            break;
        case DS_DOWNLOADING:
            StepDownloadPatchBegin();
            break;
        case DS_PATCHING:
            StepPatchBegin();
            break;
        case DS_CANCELLING:
            switch (oldState)
            {
            case DS_CHECKING_INFO:
                StepCheckInfoCancel();
                break;
            case DS_CHECKING_META:
                StepCheckMetaCancel();
                break;
            case DS_CHECKING_PATCH:
                StepCheckPatchCancel();
                break;
            case DS_READY:
                break;
            case DS_DOWNLOADING:
                StepDownloadPatchCancel();
                break;
            case DS_PATCHING:
                StepPatchCancel();
                break;
            default:
                Logger::ErrorToFile(logsFilePath, "[DLC::FSM] Unhanded state %d canceling\n", oldState);
                DVASSERT(false);
                break;
            }
            break;
        case DS_CLEANING:
            StepClean();
            break;
        case DS_DONE:
            if (dlcError == DE_CONNECT_ERROR)
            {
                // If some connection errors were occurred
                // we should check if some resources
                // already exist and also check that thous
                // resources were downloaded by game with
                // same version as current request.
                if (dlcContext.gameVer == dlcContext.localGameVer)
                {
                    // If so, we already have resources for current game version
                    // and we can safely continue to work even without DLC server
                    dlcError = DE_NO_ERROR;
                }
            }
            StepDone();
            break;
        default:
            break;
        }
    }
}

void DLC::OnDownloadTaskStateChanged(uint32 id, DownloadStatus status)
{
    if (id == dlcContext.remoteVerDownloadId)
    {
        StepCheckInfoFinish(id, status);
    }
    else if (id == dlcContext.remoteFullSizeDownloadId)
    {
        // This task implicitly handled in StepCheckPatchFinish after finishing dlcContext.remoteLiteSizeDownloadId
    }
    else if (id == dlcContext.remoteLiteSizeDownloadId)
    {
        StepCheckPatchFinish(id, status);
    }
    else if (id == dlcContext.remoteMetaDownloadId)
    {
        StepCheckMetaFinish(id, status);
    }
    else if (id == dlcContext.remotePatchDownloadId)
    {
        StepDownloadPatchFinish(id, status);
    }
}

// start downloading remote DLC version
void DLC::StepCheckInfoBegin()
{
    // write current dlcState into state-file
    if (!WriteValue(dlcContext.stateInfoStorePath, dlcState))
    {
        dlcContext.lastErrno = errno;
        Logger::ErrorToFile(logsFilePath, "[DLC::StepCheckInfoBegin] Can't write dlcState %d to stateInfoStorePath = %s", dlcState, dlcContext.stateInfoStorePath.GetAbsolutePathname().c_str());
        PostError(DE_WRITE_ERROR);
        return;
    }

    if (!dlcContext.forceFullUpdate)
    {
        ReadValue(dlcContext.localVerStorePath, &dlcContext.localVer, &dlcContext.localGameVer);
    }

    Logger::InfoToFile(logsFilePath, "[DLC::StepCheckInfoBegin] Downloading game-info\n\tfrom: %s\n\tto: %s", dlcContext.remoteVerUrl.c_str(), dlcContext.remoteVerStotePath.GetAbsolutePathname().c_str());

    DownloadManager* dm = DownloadManager::Instance();
    dlcContext.remoteVerDownloadId = dm->Download(dlcContext.remoteVerUrl, dlcContext.remoteVerStotePath.GetAbsolutePathname(), FULL, 1);
}

// downloading DLC version file finished. need to read removeVersion
void DLC::StepCheckInfoFinish(uint32 id, DownloadStatus status)
{
    if (id == dlcContext.remoteVerDownloadId)
    {
        if (DL_FINISHED == status)
        {
            DownloadError downloadError;
            DownloadManager::Instance()->GetError(id, downloadError);

            if (DLE_NO_ERROR == downloadError && FileSystem::Instance()->Exists(dlcContext.remoteVerStotePath))
            {
                if (ReadValue(dlcContext.remoteVerStotePath, &dlcContext.remoteVer))
                {
                    PostEvent(EVENT_CHECK_OK);
                }
                else
                {
                    dlcContext.lastErrno = errno;
                    Logger::ErrorToFile(logsFilePath, "[DLC::StepCheckInfoFinish] Can't read remoteVer from %s", dlcContext.remoteVerStotePath.GetAbsolutePathname().c_str());
                    PostError(DE_READ_ERROR);
                }
            }
            else
            {
                Logger::FrameworkDebugToFile(logsFilePath, "DLC: error %d", downloadError);
                if (DLE_COULDNT_RESOLVE_HOST == downloadError || DLE_COULDNT_CONNECT == downloadError || DLE_CONTENT_NOT_FOUND == downloadError)
                {
                    // connection problem
                    Logger::ErrorToFile(logsFilePath, "[DLC::StepCheckInfoFinish] Can't connect to remote host.");
                    PostError(DE_CONNECT_ERROR);
                }
                else if (DLE_FILE_ERROR == downloadError)
                {
                    // writing file problem
                    dlcContext.lastErrno = errno;
                    Logger::ErrorToFile(logsFilePath, "[DLC::StepCheckInfoFinish] Can't write to info file.");
                    PostError(DE_WRITE_ERROR);
                }
                else
                {
                    // some other unexpected error during check process
                    Logger::ErrorToFile(logsFilePath, "[DLC::StepCheckInfoFinish] Unexpected error.");
                    PostError(DE_CHECK_ERROR);
                }
            }
        }
    }
}

void DLC::StepCheckInfoCancel()
{
    DownloadManager::Instance()->Cancel(dlcContext.remoteVerDownloadId);
}

void DLC::StepCheckPatchBegin()
{
    File* f = File::Create(dlcContext.remotePatchStorePath, File::OPEN | File::READ);
    if (nullptr != f)
    {
        dlcContext.remotePatchReadySize = f->GetSize();
        f->Release();
    }

    dlcContext.remotePatchLiteSize = 0;
    dlcContext.remotePatchFullSize = 0;

    dlcContext.remotePatchFullUrl = dlcContext.remoteUrl + MakePatchUrl(0, dlcContext.remoteVer);
    dlcContext.remotePatchLiteUrl = dlcContext.remoteUrl + MakePatchUrl(dlcContext.localVer, dlcContext.remoteVer);

    Logger::InfoToFile(logsFilePath, "[DLC::StepCheckPatchBegin] Retrieving full-patch size from: %s", dlcContext.remotePatchLiteUrl.c_str());
    Logger::InfoToFile(logsFilePath, "[DLC::StepCheckPatchBegin] Retrieving lite-patch size from: %s", dlcContext.remotePatchFullUrl.c_str());

    DownloadManager* dm = DownloadManager::Instance();
    dlcContext.remoteFullSizeDownloadId = dm->Download(dlcContext.remotePatchFullUrl, dlcContext.remotePatchStorePath, GET_SIZE); // full size should be first
    dlcContext.remoteLiteSizeDownloadId = dm->Download(dlcContext.remotePatchLiteUrl, dlcContext.remotePatchStorePath, GET_SIZE); // lite size should be last
}

void DLC::StepCheckPatchFinish(uint32 id, DownloadStatus status)
{
    if (id == dlcContext.remoteLiteSizeDownloadId)
    {
        if (DL_FINISHED == status)
        {
            DownloadStatus statusFull;
            DownloadError downloadErrorFull;
            DownloadError downloadErrorLite;

            DownloadManager::Instance()->GetStatus(dlcContext.remoteFullSizeDownloadId, statusFull);
            DownloadManager::Instance()->GetError(dlcContext.remoteFullSizeDownloadId, downloadErrorFull);
            DownloadManager::Instance()->GetError(dlcContext.remoteLiteSizeDownloadId, downloadErrorLite);

            // when lite id finishing, full id should be already finished
            DVASSERT(DL_FINISHED == statusFull);

            DownloadManager::Instance()->GetTotal(dlcContext.remoteLiteSizeDownloadId, dlcContext.remotePatchLiteSize);
            DownloadManager::Instance()->GetTotal(dlcContext.remoteFullSizeDownloadId, dlcContext.remotePatchFullSize);

            if (DLE_NO_ERROR == downloadErrorLite)
            {
                dlcContext.remotePatchUrl = dlcContext.remotePatchLiteUrl;
                dlcContext.remotePatchSize = dlcContext.remotePatchLiteSize;

                PostEvent(EVENT_CHECK_OK);
            }
            else
            {
                if (DLE_NO_ERROR == downloadErrorFull)
                {
                    dlcContext.remotePatchUrl = dlcContext.remotePatchFullUrl;
                    dlcContext.remotePatchSize = dlcContext.remotePatchFullSize;

                    PostEvent(EVENT_CHECK_OK);
                }
                else
                {
                    if (DLE_COULDNT_RESOLVE_HOST == downloadErrorFull || DLE_COULDNT_CONNECT == downloadErrorFull || DLE_CONTENT_NOT_FOUND == downloadErrorFull)
                    {
                        // connection problem
                        Logger::ErrorToFile(logsFilePath, "[DLC::StepCheckPatchFinish] Can't connect.");
                        PostError(DE_CONNECT_ERROR);
                    }
                    else if (DLE_FILE_ERROR == downloadErrorFull)
                    {
                        // writing file problem
                        dlcContext.lastErrno = errno;
                        Logger::ErrorToFile(logsFilePath, "[DLC::StepCheckPatchFinish] Can't write patch to %s.", dlcContext.remotePatchUrl.c_str());
                        PostError(DE_WRITE_ERROR);
                    }
                    else
                    {
                        // some other unexpected error during check process
                        Logger::ErrorToFile(logsFilePath, "[DLC::StepCheckPatchFinish] Unexpected download error: %u.", downloadErrorFull);
                        PostError(DE_CHECK_ERROR);
                    }
                }
            }
        }
    }
}

void DLC::StepCheckPatchCancel()
{
    DownloadManager::Instance()->Cancel(dlcContext.remoteFullSizeDownloadId);
    DownloadManager::Instance()->Cancel(dlcContext.remoteLiteSizeDownloadId);
}

void DLC::StepCheckMetaBegin()
{
    dlcContext.remoteMetaUrl = dlcContext.remotePatchUrl + ".meta";

    Logger::InfoToFile(logsFilePath, "[DLC::StepCheckMetaBegin] Downloading game-meta\n\tfrom: %s\n\tto :%s", dlcContext.remoteMetaUrl.c_str(), dlcContext.remoteMetaStorePath.GetAbsolutePathname().c_str());

    FileSystem::Instance()->DeleteFile(dlcContext.remoteMetaStorePath);

    DownloadManager* dm = DownloadManager::Instance();
    dlcContext.remoteMetaDownloadId = dm->Download(dlcContext.remoteMetaUrl, dlcContext.remoteMetaStorePath, FULL, 1);
}

void DLC::StepCheckMetaFinish(uint32 id, DownloadStatus status)
{
    if (id == dlcContext.remoteMetaDownloadId)
    {
        if (DL_FINISHED == status)
        {
            DownloadError downloadError;
            DownloadManager::Instance()->GetError(dlcContext.remoteMetaDownloadId, downloadError);

            if (DLE_COULDNT_RESOLVE_HOST == downloadError || DLE_COULDNT_CONNECT == downloadError || DLE_CONTENT_NOT_FOUND == downloadError)
            {
                // connection problem
                Logger::ErrorToFile(logsFilePath, "[DLC::StepCheckMetaFinish] Can't connect do download Meta.");
                PostError(DE_CONNECT_ERROR);
            }
            else if (DLE_FILE_ERROR == downloadError)
            {
                // writing file problem
                dlcContext.lastErrno = errno;
                Logger::ErrorToFile(logsFilePath, "[DLC::StepCheckMetaFinish] Can't save Meta.");
                PostError(DE_WRITE_ERROR);
            }
            else
            {
                // any other error should be thread as OK-status, when retrieving meta-info file
                PostEvent(EVENT_CHECK_OK);
            }
        }
    }
}

void DLC::StepCheckMetaCancel()
{
    DownloadManager::Instance()->Cancel(dlcContext.remoteMetaDownloadId);
}

// download patch file
void DLC::StepDownloadPatchBegin()
{
    // write current dlcState into state-file
    if (!WriteValue(dlcContext.stateInfoStorePath, dlcState))
    {
        dlcContext.lastErrno = errno;
        Logger::ErrorToFile(logsFilePath, "[DLC::StepDownloadPatchBegin] Can't save dlcState info file.");
        PostError(DE_WRITE_ERROR);
        return;
    }

    // what mode should be used for download?
    // by default - full download
    DownloadType downloadType = FULL;

    // check what URL was downloaded last time
    File* downloadInfoFile = File::Create(dlcContext.downloadInfoStorePath, File::OPEN | File::READ);
    if (NULL != downloadInfoFile)
    {
        String lastUrl;
        String lastSizeStr;
        uint32 lastSize;

        downloadInfoFile->ReadString(lastSizeStr);
        downloadInfoFile->ReadString(lastUrl);

        lastSize = atoi(lastSizeStr.c_str());

        // if last url is same as current full or lite url we should continue downloading it
        if ((lastUrl == dlcContext.remotePatchFullUrl && lastSize == dlcContext.remotePatchFullSize) ||
            (lastUrl == dlcContext.remotePatchLiteUrl && lastSize == dlcContext.remotePatchLiteSize))
        {
            dlcContext.remotePatchUrl = lastUrl;
            dlcContext.remotePatchSize = lastSize;

            // now we can resume last download
            downloadType = RESUMED;
        }
        else
        {
            // ensure that there is no already downloaded file with another version
            FileSystem::Instance()->DeleteFile(dlcContext.remotePatchStorePath);
        }

        SafeRelease(downloadInfoFile);
    }

    if (downloadType != RESUMED) //if 'RESUMED' downloadInfoFile contains correct info and we don't want to recreate it to prevent issues when disk is full
    {
        // save URL that we gonna download
        downloadInfoFile = File::Create(dlcContext.downloadInfoStorePath, File::CREATE | File::WRITE);
        if (NULL != downloadInfoFile)
        {
            String sizeStr = Format("%u", dlcContext.remotePatchSize);
            downloadInfoFile->WriteString(sizeStr);
            downloadInfoFile->WriteString(dlcContext.remotePatchUrl);
            SafeRelease(downloadInfoFile);
        }
    }

    Logger::InfoToFile(logsFilePath, "[DLC::StepDownloadPatchBegin] Downloading patch-file\n\tfrom: %s\n\tto: %s", dlcContext.remotePatchUrl.c_str(), dlcContext.remotePatchStorePath.GetAbsolutePathname().c_str());

    // start download, notifications are handled in StepDownloadPatchFinish
    DownloadManager* dm = DownloadManager::Instance();
    dlcContext.remotePatchDownloadId = dm->Download(dlcContext.remotePatchUrl, dlcContext.remotePatchStorePath.GetAbsolutePathname(), downloadType, -1, 60); // partsCount = -1 (auto), timeout = 60 sec
}

void DLC::StepDownloadPatchFinish(uint32 id, DownloadStatus status)
{
    if (id == dlcContext.remotePatchDownloadId)
    {
        if (DL_FINISHED == status)
        {
            DownloadError downloadError;
            int32 implError;

            DownloadManager::Instance()->GetError(id, downloadError);
            DownloadManager::Instance()->GetImplError(id, implError);

            switch (downloadError)
            {
            case DAVA::DLE_NO_ERROR:
                Logger::InfoToFile(logsFilePath, "[DLC::StepDownloadPatchFinish] Downloaded succesfully.");
                //we want to have this switch from DS_DOWNLOADING to DS_PATCHING when app is in background,
                //that's why we call FSM() directly instead of using job in PostEvent()
                FSM(EVENT_DOWNLOAD_OK);
                break;

            case DAVA::DLE_COULDNT_RESOLVE_HOST:
            case DAVA::DLE_COULDNT_CONNECT:
                // connection problem
                Logger::ErrorToFile(logsFilePath, "[DLC::StepDownloadPatchFinish] Connection error at patch downloading.");
                PostError(DE_CONNECT_ERROR);
                break;

            case DAVA::DLE_FILE_ERROR:
                // writing file problem
                DownloadManager::Instance()->GetFileErrno(id, dlcContext.lastErrno);
                Logger::ErrorToFile(logsFilePath, "[DLC::StepDownloadPatchFinish] Can't write patch. File error %d", dlcContext.lastErrno);
                PostError(DE_WRITE_ERROR);
                break;

            default:
                // some other unexpected error during download process
                Logger::ErrorToFile(logsFilePath, "[DLC::StepDownloadPatchFinish] Unexpected download error: %u, implError %u.", downloadError, implError);
                PostError(DE_DOWNLOAD_ERROR);
                break;
            }
        }
    }
}

void DLC::StepDownloadPatchCancel()
{
    DownloadManager::Instance()->Cancel(dlcContext.remotePatchDownloadId);
}

void DLC::StepPatchBegin()
{
    // write current dlcState into state-file
    if (!WriteValue(dlcContext.stateInfoStorePath, dlcState))
    {
        PostError(DE_WRITE_ERROR);
        return;
    }

    dlcContext.lastErrno = 0;
    dlcContext.patchingError = PatchFileReader::ERROR_NO;
    dlcContext.lastPatchingErrorDetails = PatchFileReader::PatchingErrorDetails();
    dlcContext.patchInProgress = true;
    dlcContext.appliedPatchCount = 0;
    dlcContext.totalPatchCount = 0;

    // read number of available patches
    PatchFileReader patchReader(dlcContext.remotePatchStorePath);
    patchReader.SetLogsFilePath(logsFilePath);
    if (patchReader.ReadFirst())
    {
        do
        {
            dlcContext.totalPatchCount++;
        }
        while (patchReader.ReadNext());
    }

    Logger::InfoToFile(logsFilePath, "[DLC::StepPatchBegin] Patching, %d files to patch", dlcContext.totalPatchCount);
    patchingThread = Thread::Create(MakeFunction(this, &DLC::PatchingThread));
    patchingThread->Start();
}

void DLC::StepPatchFinish()
{
    bool errors = true;

    patchingThread->Join();
    SafeRelease(patchingThread);

    dlcContext.prevState = 0;
    dlcContext.localVer = -1;

    switch (dlcContext.patchingError)
    {
    case PatchFileReader::ERROR_NO:
        errors = false;
        PostEvent(EVENT_PATCH_OK);
        break;

    case PatchFileReader::ERROR_CANT_READ:
        PostError(DE_READ_ERROR);
        break;

    case PatchFileReader::ERROR_NEW_CREATE:
    case PatchFileReader::ERROR_NEW_WRITE:
        PostError(DE_WRITE_ERROR);
        break;

    default:
        if (!dlcContext.remotePatchUrl.empty() && dlcContext.remotePatchUrl == dlcContext.remotePatchFullUrl)
        {
            Logger::ErrorToFile(logsFilePath, "[DLC::StepPatchFinish] Can't apply full patch. patching error: %d", dlcContext.patchingError);
            PostError(DE_PATCH_ERROR_FULL);
        }
        else
        {
            Logger::ErrorToFile(logsFilePath, "[DLC::StepPatchFinish] Can't apply lite patch.");
            PostError(DE_PATCH_ERROR_LITE);
        }
        break;
    }

    if (errors)
    {
        Logger::ErrorToFile(logsFilePath, "[DLC::StepPatchFinish] Error applying patch: %u, errno %u", dlcContext.patchingError, dlcContext.lastErrno);
    }
}

void DLC::StepPatchCancel()
{
    dlcContext.patchInProgress = false;
    patchingThread->Join();
}

void DLC::PatchingThread()
{
    Logger::InfoToFile(logsFilePath, "[DLC::PatchingThread] Patching thread started");
    PatchFileReader patchReader(dlcContext.remotePatchStorePath, false, true);
    patchReader.SetLogsFilePath(logsFilePath);
    Logger::InfoToFile(logsFilePath, "[DLC::PatchingThread] PatchReader created");
    bool applySuccess = true;
    const PatchInfo* patchInfo = nullptr;

    auto applyPatchesFn = [&](bool allowTruncate, std::function<bool(const PatchInfo* info)> conditionFn)
    {
        if (applySuccess)
        {
            bool truncate = false;

            // To be able to truncate patch-file we should go from last patch to the first one.
            // If incoming patch-file doesn't support reverse pass we will try to apply it without truncation.
            // This will allow us to support old patch files.
            if (allowTruncate && patchReader.ReadLast())
            {
                truncate = true;
            }

            if (!truncate)
            {
                patchReader.ReadFirst();
            }

            // Get current patch info
            patchInfo = patchReader.GetCurInfo();
            while (applySuccess && nullptr != patchInfo && dlcContext.patchInProgress)
            {
                // Patch will be applied only if it fit condition, specified by caller
                if (conditionFn(patchInfo))
                {
                    applySuccess = patchReader.Apply(dlcContext.localSourceDir, FilePath(), dlcContext.localDestinationDir, FilePath());
                    if (applySuccess)
                    {
                        dlcContext.appliedPatchCount++;
                    }
                }

                // Go to the next patch and check if we need to truncate applied patch
                if (applySuccess && dlcContext.patchInProgress)
                {
                    if (truncate)
                    {
                        patchReader.Truncate();
                        patchReader.ReadPrev();
                    }
                    else
                    {
                        patchReader.ReadNext();
                    }

                    patchInfo = patchReader.GetCurInfo();
                }
            }
        }
    };

    // first step - apply patches, that either reduce or don't change resources size
    applyPatchesFn(false,
                   [](const PatchInfo* info)
                   {
                       return info->newSize <= info->origSize;
                   });

    // no errors on first step - continue applying patches, that increase resources size
    applyPatchesFn(true,
                   [](const PatchInfo* info)
                   {
                       return info->newSize > info->origSize;
                   });

    // check if no errors occurred during patching
    dlcContext.lastErrno = patchReader.GetFileError();
    dlcContext.patchingError = patchReader.GetError();
    dlcContext.lastPatchingErrorDetails = patchReader.GetLastErrorDetails();

    if (dlcContext.patchInProgress && PatchFileReader::ERROR_NO == dlcContext.patchingError)
    {
        DVASSERT(dlcContext.appliedPatchCount == dlcContext.totalPatchCount);

        // ensure directory, where resource version file should be, exists
        FileSystem::Instance()->CreateDirectory(dlcContext.localVerStorePath.GetDirectory(), true);

        // update local version
        if (!WriteValue(dlcContext.localVerStorePath, dlcContext.remoteVer, dlcContext.gameVer))
        {
            // error, version can't be written
            dlcContext.patchingError = PatchFileReader::ERROR_NEW_WRITE;
            dlcContext.lastPatchingErrorDetails = PatchFileReader::PatchingErrorDetails();
            dlcContext.lastErrno = errno;
        }

        // Clean patch file if it was fully truncated.
        // If we don't do that - we will have "Empty Patch Error" when patching will be finished
        // in background and application will be closed.
        File* patchFile = File::Create(dlcContext.remotePatchStorePath, File::OPEN | File::READ);
        int32 patchSizeAfterPatching = static_cast<int32>(patchFile->GetSize());
        SafeRelease(patchFile);

        if (0 == patchSizeAfterPatching)
        {
            FileSystem::Instance()->DeleteFile(dlcContext.remotePatchStorePath);
        }
    }

    Function<void()> fn(this, &DLC::StepPatchFinish);
    GetEngineContext()->jobManager->CreateMainJob(fn);
}

void DLC::StepClean()
{
    Logger::InfoToFile(logsFilePath, "[DLC::StepClean] Cleaning");

    FileSystem::Instance()->DeleteFile(dlcContext.downloadInfoStorePath);
    FileSystem::Instance()->DeleteFile(dlcContext.remoteMetaStorePath);
    FileSystem::Instance()->DeleteFile(dlcContext.remotePatchStorePath);

    PostEvent(EVENT_CLEAN_OK);
}

void DLC::StepDone()
{
    if (DE_NO_ERROR == dlcError)
    {
        FileSystem::Instance()->DeleteFile(dlcContext.remoteVerStotePath);
        FileSystem::Instance()->DeleteFile(dlcContext.stateInfoStorePath);
    }

    Logger::InfoToFile(logsFilePath, "[DLC::StepDone] Done!");
}

bool DLC::ReadValue(const FilePath& path, uint32* value, String* version)
{
    bool ret = false;

    // check if there is some unfinished download
    File* f = File::Create(path, File::OPEN | File::READ);
    if (NULL != f)
    {
        char8 tmp[64];

        tmp[0] = 0;
        if (f->ReadLine(tmp, sizeof(tmp)) > 0)
        {
            if (sscanf(tmp, "%u", value) > 0)
            {
                ret = true;
            }
        }

        if (nullptr != version)
        {
            *version = f->ReadLine();
        }

        SafeRelease(f);
    }

    return ret;
}

bool DLC::WriteValue(const FilePath& path, uint32 value, const String& version)
{
    bool ret = false;

    // check if there is some unfinished download
    File* f = File::Create(path, File::CREATE | File::WRITE);
    if (NULL != f)
    {
        String tmp = Format("%u", value);
        ret = f->WriteLine(tmp);

        if (ret && !version.empty())
        {
            ret = f->WriteLine(version);
        }

        SafeRelease(f);
    }

    return ret;
}

String DLC::MakePatchUrl(uint32 localVer, uint32 remoteVer)
{
    String ret;

    eGPUFamily gpu = DeviceInfo::GetGPUFamily();
    if (gpu < GPU_FAMILY_COUNT)
    {
        String gpuString = GPUFamilyDescriptor::GetGPUName(gpu);
        ret = Format("/r%u/r%u-%u.%s.patch", remoteVer, localVer, remoteVer, gpuString.c_str());
    }
    else
    {
        ret = Format("/r%u/r%u-%u.patch", remoteVer, localVer, remoteVer);
    }

    return ret;
}

String DLC::ToString(DownloadError e)
{
    String errorMsg;
    switch (e)
    {
    case DLE_CANCELLED: // download was canceled by our side
        errorMsg = "DLE_CANCELLED";
        break;
    case DLE_COULDNT_RESUME: // seems server doesn't supports download resuming
        errorMsg = "DLE_COULDNT_RESUME";
        break;
    case DLE_COULDNT_RESOLVE_HOST: // DNS request failed and we cannot to take IP from full qualified domain name
        errorMsg = "DLE_COULDNT_RESOLVE_HOST";
        break;
    case DLE_COULDNT_CONNECT: // we cannot connect to given address at given port
        errorMsg = "DLE_COULDNT_CONNECT";
        break;
    case DLE_CONTENT_NOT_FOUND: // server replies that there is no requested content
        errorMsg = "DLE_CONTENT_NOT_FOUND";
        break;
    case DLE_NO_RANGE_REQUEST: // Range requests is not supported. Use 1 thread without reconnects only.
        errorMsg = "DLE_NO_RANGE_REQUEST";
        break;
    case DLE_COMMON_ERROR: // some common error which is rare and requires to debug the reason
        errorMsg = "DLE_COMMON_ERROR";
        break;
    case DLE_INIT_ERROR: // any handles initialization was unsuccessful
        errorMsg = "DLE_INIT_ERROR";
        break;
    case DLE_FILE_ERROR: // file read and write errors
        errorMsg = "DLE_FILE_ERROR";
        break;
    case DLE_UNKNOWN: // we cannot determine the error
        errorMsg = "DLE_UNKNOWN";
        break;
    case DLE_NO_ERROR:
    {
        errorMsg = "DLE_NO_ERROR";
        break;
    }
    default:
        DVASSERT(false);
        break;
    } // end switch downloadError
    return errorMsg;
}

} // end namespace DAVA
