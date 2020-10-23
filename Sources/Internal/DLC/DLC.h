#ifndef __DAVAENGINE_DLC_H__
#define __DAVAENGINE_DLC_H__

#include "Base/BaseTypes.h"
#include "Base/Token.h"
#include "Concurrency/Thread.h"
#include "Downloader/DownloaderCommon.h"
#include "Patcher/PatchFile.h"

namespace DAVA
{
class DLC
{
public:
    enum DLCError
    {
        DE_NO_ERROR = 0,

        DE_WAS_CANCELED, // DLC was canceled
        DE_INIT_ERROR, // Some unhanded errors occurred during DLC initialization
        DE_DOWNLOAD_ERROR, // Some unhanded errors occurred during DLC download step
        DE_PATCH_ERROR_LITE, // Some unhanded errors occurred during applying lite patch (user can still try to force full patch applying)
        DE_PATCH_ERROR_FULL, // Some unhanded errors occurred during applying full patch
        DE_CHECK_ERROR, // There is no DLC info or DLC patch on remote host
        DE_READ_ERROR, // Can't read file
        DE_WRITE_ERROR, // Can't write file
        DE_CONNECT_ERROR // Can't connect to remote host
    };

    enum DLCState
    {
        DS_INIT,
        DS_CHECKING_INFO,
        DS_CHECKING_PATCH,
        DS_CHECKING_META,
        DS_READY,
        DS_DOWNLOADING,
        DS_PATCHING,
        DS_CANCELLING,
        DS_CLEANING,
        DS_DONE
    };

    /**
        \brief Create DLC state machine, that will check or apply patch from given URL.
        \param[in] url - remote server url.
        \param[in] sourceDir - local directory, containing original files, that should be patched.
        \param[in] desrinationDir - local directory, where patched files should be stored. Can be the same as sourceDir.
        \param[in] workingDir - local directory, where DLC temporary files should be stored.
        \param[in] gameVersion - current game version. Depending on this parameter, will be searched DLC info-file on a given (by URL parameter) remote server.
        \param[in] resVersionPath - path to file, where resources version is stored. This file will be re-wrote after patch finished.
        \param[in] forceFullUpdate - "true" value will force full-patch to be downloaded from the server. "false" leaves patch version to be determined automatically.
    */
    DLC(const String& url, const FilePath& sourceDir, const FilePath& destinationDir, const FilePath& workingDir, const String& gameVersion, const FilePath& resVersionPath, bool forceFullUpdate = false);
    ~DLC();

    /**
		\brief Starts DLC check-executing process, which will continue until the information from the remote server is received.
        DLC state machine will go into the READY state if update is required and there were no error, or in DONE state if no updates are required or there were some error.
    */
    void Check();

    /**
		\brief Starts DLC full-executing process. DLC state machine will go into the DONE state, when process finished with or without errors.
    */
    void Start();

    /**
        \brief Stops the current DLC process. DLC state machine will go into the DONE state, with error DE_WAS_CANCELED
    */
    void Cancel();

    /**
        \brief Returns the progress for current DLC state.
        \param[out] cur - current progress value
        \param[out] total - expected total progress value
    */
    void GetProgress(uint64& cur, uint64& total) const;

    /**
        \brief Returns current DLC state.
    */
    DLCState GetState() const;

    /**
        \brief Returns DLC state machine error.
    */
    DLCError GetError() const;

    /**
    \brief Return errno from patching process
    */
    int32 GetLastErrno() const;

    /**
        \brief Return error details from patching process
    */
    PatchFileReader::PatchingErrorDetails GetLastErrorInfo() const;

    /**
        \brief Return patching error
    */
    PatchFileReader::PatchError GetPatchError() const;

    /**
        \brief Returns path to appropriate meta-file that was downloaded from DLC server.
    */
    FilePath GetMetaStorePath() const;

    /**
        \brief Specifies filename for DLC system logs.
        \param[in] customLogFileName - output logs file name
    */
    void SetLogFileName(const FilePath& customLogFileName);

    static String ToString(DownloadError e);

protected:
    enum DLCEvent
    {
        EVENT_ERROR,
        EVENT_CANCEL,
        EVENT_CHECK_START,
        EVENT_CHECK_ONLY,
        EVENT_CHECK_OK,
        EVENT_DOWNLOAD_START,
        EVENT_DOWNLOAD_OK,
        EVENT_PATCH_START,
        EVENT_PATCH_OK,
        EVENT_CLEAN_OK
    };

    struct DLCContext
    {
        String remoteUrl;
        String gameVer;
        uint32 localVer;
        String localGameVer;
        bool forceFullUpdate;

        FilePath localWorkingDir;
        FilePath localSourceDir;
        FilePath localDestinationDir;
        FilePath localVerStorePath;

        String remoteVerUrl;
        uint32 remoteVer;
        uint32 remoteVerDownloadId;
        uint32 remoteFullSizeDownloadId;
        uint32 remoteLiteSizeDownloadId;
        uint32 remoteMetaDownloadId;
        FilePath remoteVerStotePath;
        FilePath remoteMetaStorePath;

        String remotePatchFullUrl;
        uint64 remotePatchFullSize;
        String remotePatchLiteUrl;
        uint64 remotePatchLiteSize;
        String remoteMetaUrl;

        String remotePatchUrl;
        uint64 remotePatchSize;
        uint64 remotePatchReadySize;
        uint32 remotePatchDownloadId;
        FilePath remotePatchStorePath;

        uint32 totalPatchCount;
        uint32 appliedPatchCount;
        volatile bool patchInProgress;
        int32 lastErrno;
        PatchFileReader::PatchError patchingError;
        PatchFileReader::PatchingErrorDetails lastPatchingErrorDetails;

        FilePath stateInfoStorePath;
        FilePath downloadInfoStorePath;
        uint32 prevState;
    };

    FilePath logsFilePath;
    DLCState dlcState;
    DLCError dlcError;
    DLCContext dlcContext;

    // FSM variables
    bool fsmAutoReady;

    // patch thread variables
    Thread* patchingThread;

    void PostEvent(DLCEvent event);
    void PostError(DLCError error);

    void FSM(DLCEvent event);

    void OnDownloadTaskStateChanged(uint32 id, DownloadStatus status);

    void StepCheckInfoBegin();
    void StepCheckInfoFinish(uint32 id, DownloadStatus status);
    void StepCheckInfoCancel();

    void StepCheckPatchBegin();
    void StepCheckPatchFinish(uint32 id, DownloadStatus status);
    void StepCheckPatchCancel();

    void StepCheckMetaBegin();
    void StepCheckMetaFinish(uint32 id, DownloadStatus status);
    void StepCheckMetaCancel();

    void StepDownloadPatchBegin();
    void StepDownloadPatchFinish(uint32 id, DownloadStatus status);
    void StepDownloadPatchCancel();

    void StepPatchBegin();
    void StepPatchFinish();
    void StepPatchCancel();

    void StepClean();
    void StepDone();

    void PatchingThread();

    // helper functions
    bool ReadValue(const FilePath& path, uint32* value, String* version = nullptr);
    bool WriteValue(const FilePath& path, uint32 value, const String& version = String());

    String MakePatchUrl(uint32 localVer, uint32 removeVer);
};
}

#endif // __DAVAENGINE_DLC_H__
