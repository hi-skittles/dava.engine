#include "CurlDownloader.h"
#include "Logger/Logger.h"
#include "FileSystem/File.h"
#include "Concurrency/Thread.h"
#include "DLCManager/Private/DLCDownloaderImpl.h"
#include "Engine/EngineContext.h"

#define CURL_STATICLIB
#include <curl/curl.h>

namespace DAVA
{
static DownloadError ErrorForEasyHandle(CURL* easyHandle, CURLcode status, bool isRangeRequestSent);
static DownloadError HandleDownloadResults(CURLM* multiHandle, bool isRangeRequestSent);
static DownloadError CurlmCodeToDownloadError(CURLMcode curlMultiCode);
static DownloadError TakeMostImportantReturnValue(const Vector<DownloadError>& errorList);

struct ErrorWithPriority
{
    DownloadError error;
    char8 priority;
};

const ErrorWithPriority errorsByPriority[] = {
    { DLE_INIT_ERROR, 0 },
    { DLE_FILE_ERROR, 1 },
    { DLE_COULDNT_RESOLVE_HOST, 2 },
    { DLE_COULDNT_CONNECT, 3 },
    { DLE_CONTENT_NOT_FOUND, 4 },
    { DLE_COMMON_ERROR, 5 },
    { DLE_UNKNOWN, 6 },
    { DLE_CANCELLED, 7 },
    { DLE_NO_RANGE_REQUEST, 8 },
    { DLE_INVALID_RANGE, 9 },
    { DLE_NO_ERROR, 10 },
};

CurlDownloader::CurlDownloader()
    : isDownloadInterrupting(false)
    , currentDownloadPartsCount(0)
    , multiHandle(nullptr)
    , storePath("")
    , downloadUrl("")
    , operationTimeout(30)
    , remoteFileSize(0)
    , sizeToDownload(0)
    , downloadSpeedLimit(0)
    , saveResult(DLE_NO_ERROR)
    , chunkInfo(nullptr)
    , saveThread(nullptr)
    , allowedBuffersInMemory(3)
    , maxChunkSize(20 * 1024 * 1024)
    , minChunkSize(16 * 1024)
{
    Context::CurlGlobalInit();
}

CurlDownloader::~CurlDownloader()
{
    Context::CurlGlobalDeinit();
}

size_t CurlDownloader::CurlDataRecvHandler(void* ptr, size_t size, size_t nmemb, void* part)
{
    DownloadPart* thisPart = static_cast<DownloadPart*>(part);
    CurlDownloader* thisDownloader = static_cast<CurlDownloader*>(thisPart->GetDownloader());

    if (thisDownloader->inactivityConnectionTimer.IsStarted())
    {
        thisDownloader->inactivityConnectionTimer.Stop();
    }

    uint32 dataLeft = thisPart->GetSize() - thisPart->GetProgress();
    size_t dataSizeCame = size * nmemb;
    uint32 dataSizeToBuffer = 0;

    if (dataLeft < dataSizeCame)
    {
        Logger::Error("[CurlDownloader::CurlDataRecvHandler] dataLeft < dataSizeCame");
        dataSizeToBuffer = dataLeft; // don't write more than part.size
    }
    else
    {
        dataSizeToBuffer = static_cast<uint32>(dataSizeCame);
    }

    if (thisPart->SaveToBuffer(static_cast<char8*>(ptr), dataSizeToBuffer))
    {
        thisDownloader->chunkInfo->progress += dataSizeToBuffer;
        thisDownloader->CalcStatistics(dataSizeToBuffer);
    }

    if (dataLeft < dataSizeCame)
    {
        // we received more data for chunk than expected, it is not a big deal.
        return dataSizeCame;
    }
    else
    {
        // no errors was found
        return static_cast<size_t>(dataSizeToBuffer);
    }
}

void CurlDownloader::Interrupt()
{
    isDownloadInterrupting = true;
}

static CURL* CurlSimpleInit()
{
    /* init the curl session */
    CURL* curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.11) Gecko/20071127 Firefox/2.0.0.11");
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    return curl_handle;
}

void CurlDownloader::SetupEasyHandle(CURL* handle, DownloadPart* part)
{
    curl_easy_setopt(handle, CURLOPT_URL, downloadUrl.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, CurlDownloader::CurlDataRecvHandler);
    if (0 < part->GetSize())
    {
        char8 rangeStr[80];
        sprintf(rangeStr, "%lld-%lld", part->GetSeekPos(), part->GetSize() + part->GetSeekPos() - 1);
        curl_easy_setopt(handle, CURLOPT_RANGE, rangeStr);
        curl_easy_setopt(handle, CURLOPT_NOBODY, 0L);
    }
    else
    {
        // we don't need to receive any data when it is unexpected
        curl_easy_setopt(handle, CURLOPT_NOBODY, 1L);
    }
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, part);

    curl_off_t speed = downloadSpeedLimit / downloadParts.size();

    curl_easy_setopt(handle, CURLOPT_MAX_RECV_SPEED_LARGE, speed);

    // set all timeouts
    SetTimeout(handle);
}

DownloadError CurlDownloader::CreateDownload()
{
    multiHandle = curl_multi_init();

    if (NULL == multiHandle)
    {
        return DLE_INIT_ERROR;
    }

    CURLMcode ret;
    for (uint8 i = 0; i < currentDownloadPartsCount; i++)
    {
        DownloadPart* part = new DownloadPart(this);
        downloadParts.push_back(part);

        /* init the curl session */
        CURL* easyHandle = CurlSimpleInit();

        if (NULL == easyHandle)
        {
            Logger::Error("[CurlDownloader::CreateDownload] Curl easy handle init error");
            return DLE_INIT_ERROR;
        }

        easyHandles.push_back(easyHandle);

        ret = curl_multi_add_handle(multiHandle, easyHandle);
        if (CURLM_OK != ret)
        {
            Logger::Error("[CurlDownloader::CreateDownload] Curl multi add handle error %d: ", ret);
            return DLE_INIT_ERROR;
        }
    }

    DVASSERT(easyHandles.size() == downloadParts.size());

    return DLE_NO_ERROR;
}

DownloadError CurlDownloader::SetupDownload(uint64 seek, uint32 size)
{
    DownloadError retCode = DLE_NO_ERROR;
    uint32 partSize = size / currentDownloadPartsCount;

    for (uint8 i = 0; i < currentDownloadPartsCount; i++)
    {
        curl_multi_remove_handle(multiHandle, easyHandles[i]);

        uint32 currentPartSize = partSize;
        if (i == currentDownloadPartsCount - 1)
        {
            // we cannot divide without errors, so we will compensate that
            currentPartSize += size - partSize * currentDownloadPartsCount;
        }

        uint64 currentPartSeekPos = seek + partSize * i;

        // we writes into one big buffer. Each chunk writes to it's part of same buffer
        downloadParts[i]->SetDestinationBuffer(chunkInfo->buffer + (i * partSize));
        downloadParts[i]->SetSize(currentPartSize);
        downloadParts[i]->SetProgress(0);
        downloadParts[i]->SetSeekPos(currentPartSeekPos);

        SetupEasyHandle(easyHandles[i], downloadParts[i]);

        if (CURLM_OK != curl_multi_add_handle(multiHandle, easyHandles[i]))
        {
            retCode = DLE_INIT_ERROR;
            break;
        }
    }

    return retCode;
}

CURLMcode CurlDownloaderPerform(CURLM*& multiHandle,
                                RawTimer& inactivityConnectionTimer,
                                long& operationTimeout,
                                bool& isDownloadInterrupting,
                                Vector<CURL*>& easyHandles)
{
    CURLMcode ret;
    int handlesRunning = 0;
    ret = curl_multi_perform(multiHandle, &handlesRunning);
    if (CURLM_OK != ret)
    {
        return ret;
    }

    inactivityConnectionTimer.Stop();
    do
    {
        struct timeval timeout;
        int rc = -1; /* select() return code */

        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;
        int32 maxfd = -1;
        long curlTimeout = -1;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        /* set a suitable timeout to play around with */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        ret = curl_multi_timeout(multiHandle, &curlTimeout);
        if (CURLM_OK != ret)
        {
            break;
        }

        if (curlTimeout >= 0)
        {
            timeout.tv_sec = curlTimeout / 1000;
            if (timeout.tv_sec > 1)
            {
                timeout.tv_sec = 1;
            }
            else
            {
                timeout.tv_usec = (curlTimeout % 1000) * 1000;
            }
        }
        /* get file descriptors from the transfers */
        ret = curl_multi_fdset(multiHandle, &fdread, &fdwrite, &fdexcep, &maxfd);
        if (CURLM_OK != ret)
            break;

        /* In a real-world program you OF COURSE check the return code of the
        function calls.  On success, the value of maxfd is guaranteed to be
        greater or equal than -1.  We call select(maxfd + 1, ...), specially in
        case of (maxfd == -1), we call select(0, ...), which is basically equal
        to sleep. */
        if (maxfd >= 0)
        {
            rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
        }
        else
        {
            // Curl documentation recommends to sleep not less than 200ms in this case
            Thread::Sleep(50); // TODO I'm think this is stupid error 200 ms is too much
            rc = 0;
        }

        switch (rc)
        {
        case -1: /* select error */
        {
            break;
        }
        case 0: /* timeout */
        {
            // workaround which allows to finish broken download on MacOS and iOS
            // operation don't interrupt on connection loss (e.g. wi-fi is turned off)
            // so we check if curl timeout is reached and then starts inactivity timer
            // timer.Reset placed inside data receive handler.
            // so if we have some data comming, timer will not reach his maximu value
            // and IsReached will be false.
            // if data don't comes - timer will reaches and we use a hack to interrupt curl by limit operation time.
            if (!inactivityConnectionTimer.IsStarted() && 0 >= curlTimeout)
            {
                inactivityConnectionTimer.Start();
            }

            uint64 timeoutOnInactivityTime = static_cast<uint64>(operationTimeout * 1000);
            uint64 inactivityTimeElapsed = inactivityConnectionTimer.GetElapsed();
            bool isTimedOut = inactivityConnectionTimer.IsStarted() && timeoutOnInactivityTime < inactivityTimeElapsed;

            if (isDownloadInterrupting || isTimedOut)
            {
                for (auto easyHandle : easyHandles)
                {
                    curl_easy_setopt(easyHandle, CURLOPT_TIMEOUT, 1L);
                }
                inactivityConnectionTimer.Stop();
            }
        }
        default: /* action */
        {
            ret = curl_multi_perform(multiHandle, &handlesRunning);
            if (CURLM_OK != ret)
            {
                return ret;
            }
        }
        }

    } while (handlesRunning > 0);

    return ret;
}

void CurlDownloader::CleanupDownload()
{
    curl_multi_cleanup(multiHandle);
    multiHandle = nullptr;

    for (DownloadPart* part : downloadParts)
    {
        SafeDelete(part);
    }
    downloadParts.clear();

    for (CURL* easy : easyHandles)
    {
        curl_easy_cleanup(easy);
    }
    easyHandles.clear();
}

void CurlDownloader::SaveChunkHandler()
{
    // TODO refactor. use semaphore to waikeup save data and wait again

    Thread* thisThread = Thread::Current();
    bool hasChunksToSave;

    do
    {
        chunksMutex.Lock();
        hasChunksToSave = !chunksToSave.empty();
        chunksMutex.Unlock();

        if (hasChunksToSave)
        {
            chunksMutex.Lock();
            DataChunkInfo* chunk = chunksToSave.front();
            if (nullptr != chunk)
            {
                chunksToSave.pop_front();
            }
            chunksMutex.Unlock();

            // TODO: Aleksei, what if chunk == nullptr?
            bool isWritten = SaveData(chunk->buffer, storePath, chunk->progress);

            SafeRelease(chunk);
            if (!isWritten)
            {
                saveResult = DLE_FILE_ERROR; // this case means that not all data which we wants to save is saved. So we produce file system error.
                fileErrno = errno;
                Logger::Error("[CurlDownloader::CurlDataRecvHandler] Couldn't save downloaded data chunk (errno=%d)", errno);
                //break - to clear chunksToSave list to prevent hang up in Download() method.
                break;
            }

            saveResult = DLE_NO_ERROR;
        }
        else
        {
            Thread::Sleep(1);
        }
    } while (hasChunksToSave || !thisThread->IsCancelling());

    chunksMutex.Lock();
    for (auto item : chunksToSave)
    {
        SafeRelease(item);
    }
    chunksToSave.clear();
    chunksMutex.Unlock();
}

DownloadError CurlDownloader::DownloadRangeOfFile(uint64 seek, uint32 size)
{
    DownloadError retCode;

    SetupDownload(seek, size);

    CURLMcode retPerform = CurlDownloaderPerform(multiHandle,
                                                 inactivityConnectionTimer,
                                                 operationTimeout,
                                                 isDownloadInterrupting,
                                                 easyHandles);

    DVASSERT(CURLM_CALL_MULTI_PERFORM != retPerform); // should not be used in curl 7.20.0 and later.

    // that is an exception from rule because of CURL interrupting mechanism.
    if (isDownloadInterrupting)
    {
        isDownloadInterrupting = false;
        return DLE_CANCELLED;
    }

    implError = retPerform;

    if (CURLM_OK == retPerform)
    {
        retCode = HandleDownloadResults(multiHandle, isRangeRequestSent);
    }
    else
    {
        retCode = CurlmCodeToDownloadError(retPerform);
    }

    return retCode;
}

DownloadError CurlDownloader::Download(const String& url, uint64 downloadOffset, uint64 downloadSize, const FilePath& savePath, uint8 partsCount, int32 timeout)
{
    Logger::FrameworkDebug("[CurlDownloader::Download]");

    operationTimeout = timeout;
    storePath = savePath;
    downloadUrl = url;
    currentDownloadPartsCount = partsCount;
    fileErrno = 0;
    implError = 0;
    DownloadError retCode = GetSize(downloadUrl, remoteFileSize, operationTimeout);

    if (DLE_NO_ERROR != retCode)
    {
        return retCode;
    }

    if (downloadSize == 0)
    {
        downloadSize = remoteFileSize;
        downloadOffset = 0;
    }

    // Check download range against file size
    if (downloadOffset + downloadSize > remoteFileSize)
    {
        return DLE_INVALID_RANGE;
    }

    uint64 currentFileSize = 0;
    // if file exists - don't reload already downloaded part, just report
    File* dstFile = File::Create(storePath, File::OPEN | File::READ);
    if (NULL != dstFile)
    {
        currentFileSize = dstFile->GetSize();
    }
    else
    {
        dstFile = File::Create(storePath, File::CREATE | File::WRITE);
        if (NULL == dstFile)
        {
            fileErrno = errno;
            return DLE_FILE_ERROR;
        }
    }
    SafeRelease(dstFile);

    // Something is wrong if already downloaded part size is greater then download size
    if (currentFileSize > downloadSize)
    {
        return DLE_INVALID_RANGE;
    }

    // File already downloaded
    if (currentFileSize == downloadSize)
    {
        return DLE_NO_ERROR;
    }

    saveResult = DLE_NO_ERROR;

    downloadSize -= currentFileSize;
    downloadOffset += currentFileSize;

    // rest part of file to download
    sizeToDownload = downloadSize;

    // reset download speed statistics
    ResetStatistics(sizeToDownload);

    uint32 inMemoryBufferChunkSize = Min<uint32>(maxChunkSize, static_cast<uint32>(downloadSize / 100));
    // a part of file to parallel download
    // cast is needed because it is garanteed that download part is lesser than 4Gb
    uint32 fileChunkSize = Max<uint32>(minChunkSize, inMemoryBufferChunkSize);
    // quantity of paralleled file parts
    // if file size is 0 - we don't need more than 1 download thread.
    // if file exists
    uint64 fileChunksCount = (0 == fileChunkSize) ? 1 : Max<uint32>(1, static_cast<uint32>(sizeToDownload / fileChunkSize));

    // Range Request is a trying to download a part of file starting from some offset.
    // File is divided into fileChunksCount chunks, so if chunk count is more than 1 - then RangeRequest will be performed
    // if we want to use more than 1 download part - then we need RangeRequest
    isRangeRequestSent = 1 < fileChunksCount || 1 < partsCount;

    // part size could not be bigger than 4Gb
    uint32 lastFileChunkSize = fileChunkSize + static_cast<uint32>(sizeToDownload - fileChunksCount * fileChunkSize);

    saveThread = Thread::Create(MakeFunction(this, &CurlDownloader::SaveChunkHandler));
    saveThread->Start();

    uint32 chunksInList = 0;

    retCode = CreateDownload();
    if (DLE_NO_ERROR == retCode)
    {
        for (uint64 i = 0; i < fileChunksCount; ++i)
        {
            // download from seek pos
            uint64 seek = downloadOffset + fileChunkSize * i;

            // last download part considers the inaccuracy of division of file to parts
            if (i == fileChunksCount - 1)
            {
                fileChunkSize = lastFileChunkSize;
            }

            chunkInfo = new DataChunkInfo(fileChunkSize);

            // download a part of file
            retCode = DownloadRangeOfFile(seek, fileChunkSize);

            if (DLE_NO_ERROR == retCode)
            {
                do
                {
                    Thread::Sleep(1);
                    chunksMutex.Lock();
                    chunksInList = static_cast<uint32>(chunksToSave.size());
                    chunksMutex.Unlock();
                    // iterate until overbuffers save. Break if we have save error.
                } while (allowedBuffersInMemory < chunksInList && DLE_NO_ERROR == saveResult);

                if (DLE_NO_ERROR != saveResult)
                {
                    retCode = saveResult;
                    SafeRelease(chunkInfo);
                }
                else
                {
                    chunksMutex.Lock();
                    chunksToSave.push_back(chunkInfo);
                    chunksMutex.Unlock();
                }
            }

            if (DLE_NO_ERROR != retCode)
            {
                break;
            }
        }
    }
    CleanupDownload();

    ResetStatistics(0);

    // wait for save of rest file part from memory
    // if data saving is slower than data downloading
    do
    {
        Thread::Sleep(1);
        chunksMutex.Lock();
        chunksInList = static_cast<uint32>(chunksToSave.size());
        chunksMutex.Unlock();
        // break if we have save error. chunks clears in saveHandler.
    } while (0 < chunksInList && DLE_NO_ERROR == saveResult);

    saveThread->Cancel();
    saveThread->Join();
    SafeRelease(saveThread);

    if (DLE_NO_ERROR != saveResult)
    {
        retCode = saveResult;
    }

    return retCode;
}

DownloadError CurlDownloader::DownloadIntoBuffer(const String& url,
                                                 uint64 downloadOffset,
                                                 uint64 downloadSize,
                                                 void* buffer,
                                                 uint32 bufSize,
                                                 uint8 partsCount,
                                                 int32 timeout,
                                                 uint32* nread)
{
    DVASSERT(nread != nullptr);
    DVASSERT(bufSize > 0 && buffer != nullptr);
    DVASSERT(downloadSize <= bufSize);

    Logger::FrameworkDebug("[CurlDownloader::Download into buffer]");

    operationTimeout = timeout;
    downloadUrl = url;
    currentDownloadPartsCount = partsCount;
    fileErrno = 0;
    implError = 0;
    DownloadError retCode = GetSize(downloadUrl, remoteFileSize, operationTimeout);
    if (DLE_NO_ERROR != retCode)
    {
        return retCode;
    }

    if (downloadSize == 0)
    {
        downloadSize = remoteFileSize;
        downloadOffset = 0;
    }

    // Check download range against file size
    if (downloadOffset + downloadSize > remoteFileSize)
    {
        return DLE_INVALID_RANGE;
    }

    downloadSize = std::min<uint64>(bufSize, downloadSize);

    saveResult = DLE_NO_ERROR;

    // rest part of file to download
    sizeToDownload = downloadSize;

    // reset download speed statistics
    ResetStatistics(sizeToDownload);

    uint32 inMemoryBufferChunkSize = Min<uint32>(maxChunkSize, static_cast<uint32>(downloadSize / 100));
    // a part of file to parallel download
    // cast is needed because it is garanteed that download part is lesser than 4Gb
    uint32 fileChunkSize = Max<uint32>(minChunkSize, inMemoryBufferChunkSize);
    // quantity of paralleled file parts
    // if file size is 0 - we don't need more than 1 download thread.
    // if file exists
    uint64 fileChunksCount = (0 == fileChunkSize) ? 1 : Max<uint32>(1, static_cast<uint32>(sizeToDownload / fileChunkSize));

    // Range Request is a trying to download a part of file starting from some offset.
    // File is divided into fileChunksCount chunks, so if chunk count is more than 1 - then RangeRequest will be performed
    // if we want to use more than 1 download part - then we need RangeRequest
    isRangeRequestSent = 1 < fileChunksCount || 1 < partsCount;

    // part size could not be bigger than 4Gb
    uint32 lastFileChunkSize = fileChunkSize + static_cast<uint32>(sizeToDownload - fileChunksCount * fileChunkSize);

    void* writeTo = buffer;
    uint32 nwritten = 0;

    retCode = CreateDownload();
    if (DLE_NO_ERROR == retCode)
    {
        for (uint64 i = 0; i < fileChunksCount; ++i)
        {
            // download from seek pos
            uint64 seek = downloadOffset + fileChunkSize * i;

            // last download part considers the inaccuracy of division of file to parts
            if (i == fileChunksCount - 1)
            {
                fileChunkSize = lastFileChunkSize;
            }

            chunkInfo = new DataChunkInfo(fileChunkSize);

            // download a part of file
            retCode = DownloadRangeOfFile(seek, fileChunkSize);
            if (DLE_NO_ERROR == retCode)
            {
                Memcpy(writeTo, chunkInfo->buffer, chunkInfo->bufferSize);
                nwritten += chunkInfo->bufferSize;
                writeTo = OffsetPointer<void*>(writeTo, chunkInfo->bufferSize);

                notifyProgress(nwritten);
            }
            SafeRelease(chunkInfo);

            if (DLE_NO_ERROR != retCode)
            {
                break;
            }
        }
    }
    *nread = nwritten;

    CleanupDownload();
    ResetStatistics(0);
    return retCode;
}

void CurlDownloader::SetDownloadSpeedLimit(const uint64 limit)
{
    downloadSpeedLimit = limit;
}

DownloadError CurlDownloader::GetSize(const String& url, uint64& retSize, int32 timeout)
{
    isRangeRequestSent = false;
    operationTimeout = timeout;
    float64 sizeToDownload = 0.0; // curl need double! do not change
    CURL* currentCurlHandle = CurlSimpleInit();

    if (!currentCurlHandle)
    {
        return DLE_INIT_ERROR;
    }

    curl_easy_setopt(currentCurlHandle, CURLOPT_HEADER, 0L);
    curl_easy_setopt(currentCurlHandle, CURLOPT_URL, url.c_str());

    // Don't return the header (we'll use curl_getinfo();
    curl_easy_setopt(currentCurlHandle, CURLOPT_NOBODY, 1L);

    // set all timeouts
    SetTimeout(currentCurlHandle);

    CURLcode curlStatus = curl_easy_perform(currentCurlHandle);
    curl_easy_getinfo(currentCurlHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &sizeToDownload);
    if (sizeToDownload < 0)
    {
        sizeToDownload = 0;
    }

    DownloadError retError = ErrorForEasyHandle(currentCurlHandle, curlStatus, isRangeRequestSent);
    retSize = static_cast<uint64>(sizeToDownload);

    implError = curlStatus;

    /* cleanup curl stuff */
    curl_easy_cleanup(currentCurlHandle);

    return retError;
}

DownloadError CurlStatusToDownloadStatus(CURLcode status)
{
    switch (status)
    {
    case CURLE_OK:
        return DLE_NO_ERROR;

    case CURLE_RANGE_ERROR:
        return DLE_COULDNT_RESUME;

    case CURLE_WRITE_ERROR: // happens if callback function for data receive returns wrong number of written data
        return DLE_FILE_ERROR;

    case CURLE_COULDNT_RESOLVE_HOST:
        return DLE_COULDNT_RESOLVE_HOST;

    case CURLE_RECV_ERROR:
    case CURLE_COULDNT_CONNECT:
    case CURLE_OPERATION_TIMEDOUT:
        return DLE_COULDNT_CONNECT;

    default:
        Logger::Error("[CurlDownloader] Unhandled curl status: %u", status);
        return DLE_COMMON_ERROR;
    }
}

static DownloadError CurlmCodeToDownloadError(CURLMcode curlMultiCode)
{
    switch (curlMultiCode)
    {
    case CURLM_OK:
        return DLE_NO_ERROR;
    case CURLM_CALL_MULTI_PERFORM:
    case CURLM_ADDED_ALREADY:
    case CURLM_BAD_HANDLE:
    case CURLM_BAD_EASY_HANDLE:
    case CURLM_OUT_OF_MEMORY:
        return DLE_INIT_ERROR;
    case CURLM_INTERNAL_ERROR:
    case CURLM_BAD_SOCKET:
    case CURLM_UNKNOWN_OPTION:
    default:
        return DLE_COMMON_ERROR;
    }
}

DownloadError HttpCodeToDownloadError(long code, bool isRangeRequestSent)
{
    HttpCodeClass code_class = static_cast<HttpCodeClass>(code / 100);
    switch (code_class)
    {
    case HTTP_CLIENT_ERROR:
    case HTTP_SERVER_ERROR:
        return DLE_CONTENT_NOT_FOUND;
    case HTTP_SUCCESS:
        if (isRangeRequestSent && 200 == code)
        {
            // Seems Server doesn't supports Range requests.
            return DLE_NO_RANGE_REQUEST;
        } // else return DLE_NO_ERROR
    default:
        return DLE_NO_ERROR;
    }
}

void CurlDownloader::SetTimeout(CURL* easyHandle)
{
    curl_easy_setopt(easyHandle, CURLOPT_TIMEOUT, 0L);

    curl_easy_setopt(easyHandle, CURLOPT_CONNECTTIMEOUT, operationTimeout);
    curl_easy_setopt(easyHandle, CURLOPT_DNS_CACHE_TIMEOUT, operationTimeout);

    // abort if slower than 30 bytes/sec during operationTimeout seconds
    curl_easy_setopt(easyHandle, CURLOPT_LOW_SPEED_TIME, 30L);
    curl_easy_setopt(easyHandle, CURLOPT_LOW_SPEED_LIMIT, operationTimeout);
}

static DownloadError HandleDownloadResults(CURLM* multiHandle, bool isRangeRequestSent)
{
    // handle easy handles states
    Vector<DownloadError> results;

    int32 messagesRest;
    do
    {
        CURLMsg* message = curl_multi_info_read(multiHandle, &messagesRest);
        if (NULL == message)
        {
            break;
        }

        results.push_back(ErrorForEasyHandle(message->easy_handle, message->data.result, isRangeRequestSent));
    } while (0 != messagesRest);

    return TakeMostImportantReturnValue(results);
}

static DownloadError ErrorForEasyHandle(CURL* easyHandle, CURLcode status, bool isRangeRequestSent)
{
    DownloadError retError;

    long httpCode; // use long! sizeof(long) == 8 on macosx
    curl_easy_getinfo(easyHandle, CURLINFO_RESPONSE_CODE, &httpCode);

    // to discuss. It is ideal to place it to DownloadManager because in that case we need to use same code inside each downloader.

    DownloadError httpError = HttpCodeToDownloadError(httpCode, isRangeRequestSent);
    if (DLE_NO_ERROR != httpError)
    {
        retError = httpError;
    }
    else
    {
        retError = CurlStatusToDownloadStatus(status);
    }

    return retError;
}

static DownloadError TakeMostImportantReturnValue(const Vector<DownloadError>& errorList)
{
    char8 errorCount = sizeof(errorsByPriority) / sizeof(ErrorWithPriority);
    int32 retIndex = errorCount - 1; // last error in the list is the less important.
    char8 priority = errorsByPriority[retIndex].priority; //priority of less important error

    // iterate over download results
    Vector<DownloadError>::const_iterator end = errorList.end();
    for (Vector<DownloadError>::const_iterator it = errorList.begin(); it != end; ++it)
    {
        // find error in the priority map
        for (int32 i = 0; i < errorCount; ++i)
        {
            // yes, that is the error
            if (errorsByPriority[i].error == (*it))
            {
                // is found error priority is less that current more important error?
                // less priority is more important
                if (priority > errorsByPriority[i].priority)
                {
                    //yes, so save more important priority and it's index
                    priority = errorsByPriority[i].priority;
                    retIndex = i;
                }
            }
        }
    }

    //return more important error by index
    return errorsByPriority[retIndex].error;
}
}
