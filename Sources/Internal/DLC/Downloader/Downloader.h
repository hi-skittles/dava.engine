#pragma once

#include "Base/BaseTypes.h"
#include "DownloaderCommon.h"
#include "Functional/Function.h"
#include "Concurrency/Spinlock.h"

namespace DAVA
{
/*
    Base class for eny downloaders. Used as interface inside DownloadManager.
*/
class Downloader
{
    /* only Download manager could use Downloader and it's childs*/
    friend class DownloadManager;

public:
    Downloader();
    virtual ~Downloader() = default;

    /* all methods putted into protected section because they should be used only from DownloadManager. */
protected:
    /**
        \brief Get content size in bytes for remote Url.
        \param[in] url - destination fie Url
        \param[out] retSize - place result to
        \param[in] timeout - operation timeout
     */
    virtual DownloadError GetSize(const String& url, uint64& retSize, int32 timeout) = 0;
    /**
        \brief Main downloading operation. Should call SaveData to store data.
        \param[in] url - destination file Url
        \param[in] downloadOffset - offset to download from, used together with contentSize parameter
        \param[in] downloadSize - size in bytes to download starting from downloadOffset; if downloadSize is zero then download full content
        \param[in] savePath - path to save location of remote file
        \param[in] partsCount - quantity of download threads
        \param[in] timeout - operation timeout
    */
    virtual DownloadError Download(const String& url, uint64 downloadOffset, uint64 downloadSize, const FilePath& savePath, uint8 partsCount, int32 timeout) = 0;
    /**
    \brief Download file and store downloaded content into provided memory buffer
    \param[in] url - destination file Url
    \param[in] downloadOffset - offset to download from, used together with contentSize parameter
    \param[in] downloadSize - size in bytes to download starting from downloadOffset; if downloadSize is zero then download full content
    \param[in] buffer - buffer for downloaded content
    \param[in] bufSize - size of buffer
    \param[in] partsCount - quantity of download threads
    \param[in] timeout - operation timeout
    \param[out] nread - number of bytes saved in buffer
    */
    virtual DownloadError DownloadIntoBuffer(const String& url,
                                             uint64 downloadOffset,
                                             uint64 downloadSize,
                                             void* buffer,
                                             uint32 bufSize,
                                             uint8 partsCount,
                                             int32 timeout,
                                             uint32* nread) = 0;

    /**
        \brief Interrupt download process. We expects that you will save last data chunk came before
     */
    virtual void Interrupt() = 0;
    /**
        \brief Main save method. Should be preferred way to store any downloaded data. If not - you can reimplement it, but it is not recommended.
        Take a look on CurlDownloader::CurlDataRecvHandler(...) for example.
        \param[in] ptr - pointer to data
        \param[in] storePath - path to save location of remote file
        \param[in] size - amount of data
        \param[in] seek - position in file where data should be stored
    */
    virtual bool SaveData(const void* ptr, const FilePath& storePath, uint64 size);
    /**
        \brief Used to report about saved data size to download manager. Used to calculate total download progress.
     */
    virtual void SetProgressNotificator(Function<void(uint64)> progressNotifier);
    /**
        \brief Reset download statistics
        \param[in] sizeToDownload - data size we suppose to download
     */
    void ResetStatistics(uint64 sizeToDownload);
    /**
        \brief Calculate download statistics. Should be called at data came or saved.
        \param[in] dataCame - amout of data came or saved.
     */
    void CalcStatistics(uint32 dataCame);
    /**
        \brief Returns download statistics structure
     */
    DownloadStatistics GetStatistics();
    /**
         \brief Sets maximum allowed download speed. 0 means unlimited.
         \param[in] limit - speed limit in bytes per second.
     */
    virtual void SetDownloadSpeedLimit(const uint64 limit) = 0;

    /** Return errno occurred during work with destination file. */
    int32 GetFileErrno() const;

    /** Return error specified for downloader implementation. Useful for debugging/tracing download errors. */
    int32 GetImplError() const;

    int32 fileErrno = 0;
    int32 implError = 0;
    Function<void(uint64)> notifyProgress;

private:
    uint64 dataToDownloadLeft = 0;

    Spinlock statisticsMutex;
    DownloadStatistics statistics;
};
}
