#pragma once

#include "Downloader.h"
#include "Time/RawTimer.h"
#include "Concurrency/Mutex.h"

extern "C"
{
typedef void CURL;
typedef void CURLM;
}

namespace DAVA
{
class Thread;

class CurlDownloader final : public Downloader
{
public:
    CurlDownloader();
    virtual ~CurlDownloader();

protected:
    /**
        \brief Interrupts current download.
     */
    void Interrupt() override;
    /**
     \brief Get content size in bytes for remote Url.
     \param[in] url - destination fie Url
     \param[out] retSize - place result to
     \param[in] timeout - operation timeout
     */
    DownloadError GetSize(const String& url, uint64& retSize, int32 timeout) override;
    /**
     \brief Main downloading operation. Should call SaveData to store data.
     \param[in] url - destination file Url
     \param[in] downloadOffset - offset to download from, used together with contentSize parameter
     \param[in] downloadSize - size in bytes to download starting from downloadOffset; if downloadSize is zero then download full content
     \param[in] savePath - path to save location of remote file
     \param[in] partsCount - quantity of download threads
     \param[in] timeout - operation timeout
     */
    DownloadError Download(const String& url, uint64 downloadOffset, uint64 downloadSize, const FilePath& savePath, uint8 partsCount, int32 timeout) override;

    DownloadError DownloadIntoBuffer(const String& url,
                                     uint64 downloadOffset,
                                     uint64 downloadSize,
                                     void* buffer,
                                     uint32 bufSize,
                                     uint8 partsCount,
                                     int32 timeout,
                                     uint32* nread) override;

    /**
     \brief Sets maximum allowed download speed. -1 means unlimited.
     \param[in] limit - speed limit in bytes per second.
     */
    void SetDownloadSpeedLimit(const uint64 limit) override;

private:
    /**
     \brief Method for save downloaded data in a separate thread
     */
    void SaveChunkHandler();
    /**
     \brief Downloads a part of file using a number of download threads
     \param[in] seek - position inside remote file to download from
     \param[in] size - size of data do download
     */
    DownloadError DownloadRangeOfFile(uint64 seek, uint32 size);
    /**
        \brief Data receive handler for all easy handles which downloads a data
        \param[in] ptr - pointer to incoming data chunk
        \param[in] size - one data buffer size
        \param[in] nmemb - quantity of came data buffers
        \param[in] part - pointer to download part which contains data for current download thread
     */
    static size_t CurlDataRecvHandler(void* ptr, size_t size, size_t nmemb, void* part);
    /**
        \brief Create one of easy handles to download content. Returns a pointer to new created curl easy handle
        \param[in] part - pointer to download part which contains data for current download thread
     */
    void SetupEasyHandle(CURL* handle, DownloadPart* part);
    /**
     \brief Init curl download handles and DownloadParts
     */
    DownloadError CreateDownload();
    /**
     \brief Prepare all we need to start or resume download
     \param[in] seek - position inside remote file to download from
     \param[in] size - size of data do download
     */
    DownloadError SetupDownload(uint64 seek, uint32 size);
    /**
        \brief Cleanup all used Curl resurces when they are not needed anymore
     */
    void CleanupDownload();
    /**
        \brief Set up Curl timeouts
        \param[in] handle - Curl easy handle to set options
     */
    void SetTimeout(CURL* easyHandle);

    bool isDownloadInterrupting;
    uint8 currentDownloadPartsCount;
    Vector<DownloadPart*> downloadParts;
    Vector<CURL*> easyHandles;
    CURLM* multiHandle;
    FilePath storePath;
    String downloadUrl;
    long operationTimeout; // curl use long (sizeof(long) == 8 on macos)
    RawTimer inactivityConnectionTimer; //-V730_NOINIT
    uint64 remoteFileSize;
    uint64 sizeToDownload;
    uint64 downloadSpeedLimit;

    DownloadError saveResult;
    DataChunkInfo* chunkInfo;
    Mutex chunksMutex;
    List<DataChunkInfo*> chunksToSave;
    Thread* saveThread = nullptr;
    const uint8 allowedBuffersInMemory;

    const uint32 maxChunkSize;
    const uint32 minChunkSize;

    bool isRangeRequestSent = false;
};
}
