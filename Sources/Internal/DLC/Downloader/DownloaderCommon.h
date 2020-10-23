#pragma once

#include "Base/BaseObject.h"
#include "FileSystem/FilePath.h"

namespace DAVA
{
class FilePath;
/*
    Task type
 */
enum DownloadType
{
    RESUMED = 0, // try to resume downllad
    FULL, // download data even if there was a downloaded part
    GET_SIZE, // just get size of remote file
};

/*
    Download task states
 */
enum DownloadStatus
{
    DL_PENDING = 0, // task is in pending queue
    DL_IN_PROGRESS, // task is performs now (means DownloadManager::currentTask is only one the task on that state)
    DL_FINISHED, // task is in finished queue
    DL_UNKNOWN, // unknow download status (means that task is just created)
};

/*
    All download errors which we handles
*/
enum DownloadError
{
    DLE_NO_ERROR = 0, // there is no errors
    DLE_CANCELLED, // download was cancelled by our side
    DLE_COULDNT_RESUME, // seems server doesn't supports download resuming
    DLE_COULDNT_RESOLVE_HOST, // DNS request failed and we cannot to take IP from full qualified domain name
    DLE_COULDNT_CONNECT, // we cannot connect to given adress at given port
    DLE_CONTENT_NOT_FOUND, // server replies that there is no requested content
    DLE_NO_RANGE_REQUEST, // Range requests is not supported. Use 1 thread without reconnects only.
    DLE_COMMON_ERROR, // some common error which is rare and requires to debug the reason
    DLE_INIT_ERROR, // any handles initialisation was unsuccessful
    DLE_FILE_ERROR, // file write errors
    DLE_UNKNOWN, // we cannot determine the error
    DLE_INVALID_RANGE, // download range is outside file or already downloaded part is greater than download size
};

/*
    HTTP code classes which should be handles by any application which works with HTTP
    You can take more information here https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
 */
enum HttpCodeClass
{
    HTTP_INFO = 1,
    HTTP_SUCCESS,
    HTTP_REDIRECTION,
    HTTP_CLIENT_ERROR,
    HTTP_SERVER_ERROR,
};

struct DownloadStatistics
{
    enum
    {
        VALUE_UNKNOWN = -1,
    };
    uint64 downloadSpeedBytesPerSec;
    uint64 timeLeftSecs;
    uint64 dataCameTotalBytes;
};
/*
    Download task information which contains all necessery data to perform download and handle any download states
 */
struct DownloadTaskDescription
{
    DownloadTaskDescription(const String& srcUrl,
                            const FilePath& storeToFilePath,
                            DownloadType downloadMode,
                            int32 _timeout,
                            int32 _retriesCount,
                            uint8 _partsCount,
                            uint64 _downloadOffset,
                            uint64 _downloadSize);

    DownloadTaskDescription(const String& srcUrl,
                            void* buffer,
                            uint32 bufSize,
                            DownloadType downloadMode,
                            int32 _timeout,
                            int32 _retriesCount,
                            uint8 _partsCount,
                            uint64 _downloadOffset,
                            uint64 _downloadSize);

    uint32 id;
    String url;
    FilePath storePath;
    int32 fileErrno;
    int32 implError;
    int32 timeout;
    int32 retriesCount;
    int32 retriesLeft;
    DownloadType type;
    DownloadStatus status;
    DownloadError error;
    uint64 downloadTotal;
    uint64 downloadProgress;
    uint8 partsCount;

    uint64 downloadOffset;
    uint64 downloadSize;
    void* memoryBuffer = nullptr;
    uint32 memoryBufferSize = 0;
    uint32 memoryBufferContentSize = 0;
};

/*
    Contains all info which we need to know at download restore
*/
struct DownloadInfoHeader
{
    uint8 partsCount;
};

class Downloader;
class DownloadPart
{
public:
    DownloadPart(Downloader* currentDownloader);

    bool SaveToBuffer(char8* srcBuf, uint32 size);

    inline void SetDestinationBuffer(char8* dstBuffer);
    inline void SetSeekPos(uint64 seek);
    inline uint64 GetSeekPos() const;
    inline void SetSize(uint32 size);
    inline uint32 GetSize() const;
    inline void SetProgress(uint32 newProgress);
    inline uint32 GetProgress() const;

    inline Downloader* GetDownloader() const;

private:
    /*
        Used to pass a pointer to current Downloader into DataReceive handler
     */
    Downloader* downloader;
    char8* dataBuffer;

    uint32 downloadSize;
    uint64 seekPos;
    uint32 progress;
};

inline void DownloadPart::SetDestinationBuffer(char8* dstBuffer)
{
    dataBuffer = dstBuffer;
}

inline void DownloadPart::SetSeekPos(uint64 seek)
{
    seekPos = seek;
}

inline uint64 DownloadPart::GetSeekPos() const
{
    return seekPos;
}

inline void DownloadPart::SetSize(uint32 size)
{
    downloadSize = size;
}

inline uint32 DownloadPart::GetSize() const
{
    return downloadSize;
}

inline void DownloadPart::SetProgress(uint32 newProgress)
{
    progress = newProgress;
}

inline uint32 DownloadPart::GetProgress() const
{
    return progress;
}

inline Downloader* DownloadPart::GetDownloader() const
{
    return downloader;
}

class DataChunkInfo : public BaseObject
{
protected:
    ~DataChunkInfo();

public:
    explicit DataChunkInfo(uint32 size);

    char8* buffer;
    uint32 bufferSize;
    uint64 progress;
};
}
