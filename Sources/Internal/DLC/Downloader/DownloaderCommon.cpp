#include "DownloaderCommon.h"
#include "Logger/Logger.h"

namespace DAVA
{
DownloadTaskDescription::DownloadTaskDescription(const String& srcUrl,
                                                 const FilePath& storeToFilePath,
                                                 DownloadType downloadMode,
                                                 int32 _timeout,
                                                 int32 _retriesCount,
                                                 uint8 _partsCount,
                                                 uint64 _downloadOffset,
                                                 uint64 _downloadSize)
    : id(0)
    , url(srcUrl)
    , storePath(storeToFilePath)
    , fileErrno(0)
    , implError(0)
    , timeout(_timeout)
    , retriesCount(_retriesCount)
    , retriesLeft(retriesCount)
    , type(downloadMode)
    , status(DL_UNKNOWN)
    , error(DLE_NO_ERROR)
    , downloadTotal(0)
    , downloadProgress(0)
    , partsCount(_partsCount)
    , downloadOffset(_downloadOffset)
    , downloadSize(_downloadSize)
{
}

DownloadTaskDescription::DownloadTaskDescription(const String& srcUrl,
                                                 void* buffer,
                                                 uint32 bufSize,
                                                 DownloadType downloadMode,
                                                 int32 _timeout,
                                                 int32 _retriesCount,
                                                 uint8 _partsCount,
                                                 uint64 _downloadOffset,
                                                 uint64 _downloadSize)
    : id(0)
    , url(srcUrl)
    , fileErrno(0)
    , implError(0)
    , timeout(_timeout)
    , retriesCount(_retriesCount)
    , retriesLeft(retriesCount)
    , type(downloadMode)
    , status(DL_UNKNOWN)
    , error(DLE_NO_ERROR)
    , downloadTotal(0)
    , downloadProgress(0)
    , partsCount(_partsCount)
    , downloadOffset(_downloadOffset)
    , downloadSize(_downloadSize)
    , memoryBuffer(buffer)
    , memoryBufferSize(bufSize)
{
}

DownloadPart::DownloadPart(Downloader* currentDownloader)
    : downloader(currentDownloader)
    , dataBuffer(nullptr)
    , downloadSize(0)
    , seekPos(0)
    , progress(0)

{
}

bool DownloadPart::SaveToBuffer(char8* srcBuf, uint32 size)
{
    DVASSERT(downloadSize - progress >= size);
    if (downloadSize - progress > 0)
    {
        Memcpy(dataBuffer + progress, srcBuf, size);
        progress += size;
        return true;
    }
    Logger::Error("[DownloadPart::SaveToBuffer] Cannot save data.");
    return false;
}

DataChunkInfo::DataChunkInfo(uint32 size)
    : buffer(new char8[size])
    , bufferSize(size)
    , progress(0)
{
}

DataChunkInfo::~DataChunkInfo()
{
    SafeDeleteArray(buffer);
    bufferSize = 0;
}
}