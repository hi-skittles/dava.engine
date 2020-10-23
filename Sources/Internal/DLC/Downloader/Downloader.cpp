#include "Downloader.h"
#include "DLC/Downloader/DownloadManager.h"
#include "Time/SystemTimer.h"
#include "Concurrency/LockGuard.h"
#include "Logger/Logger.h"
#include "FileSystem/File.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
Downloader::Downloader()
{
}

bool Downloader::SaveData(const void* ptr, const FilePath& storePath, uint64 size)
{
    size_t written = 0;
    File* destFile = File::Create(storePath, File::OPEN | File::WRITE | File::APPEND);
    if (destFile)
    {
        DownloadManager::Instance()->ResetRetriesCount();
#if defined(__DAVAENGINE_ANDROID__)
        uint32 posBeforeWrite = destFile->GetPos();
#endif
        written = destFile->Write(ptr, static_cast<uint32>(size)); // only 32 bit write is supported

#if defined(__DAVAENGINE_ANDROID__)
        //for Android value returned by 'Write()' is incorrect in case of full disk, that's why we calculate 'written' using 'GetPos()'
        DVASSERT(destFile->GetPos() >= posBeforeWrite);
        written = destFile->GetPos() - posBeforeWrite;
#endif
        SafeRelease(destFile);

        notifyProgress(written);

        if (written != size)
        {
            Logger::Error("[Downloader::SaveData] Cannot save data to the file");
            return false;
        }
    }
    else
    {
        Logger::Error("[Downloader::SaveData] Cannot open file to save data");
        return false;
    }

    return true;
}

void Downloader::SetProgressNotificator(Function<void(uint64)> progressNotifier)
{
    notifyProgress = progressNotifier;
}

void Downloader::ResetStatistics(uint64 sizeToDownload)
{
    dataToDownloadLeft = sizeToDownload;
    statistics.downloadSpeedBytesPerSec = 0;
    statistics.timeLeftSecs = static_cast<uint64>(DownloadStatistics::VALUE_UNKNOWN);
    statistics.dataCameTotalBytes = 0;
}

void Downloader::CalcStatistics(uint32 dataCame)
{
    DVASSERT(dataToDownloadLeft >= dataCame);
    dataToDownloadLeft -= dataCame;

    static uint64 curTime = SystemTimer::GetMs();
    static uint64 prevTime = curTime;
    static uint64 timeDelta = 0;

    static uint64 dataSizeCame = 0;
    dataSizeCame += dataCame;

    curTime = SystemTimer::GetMs();
    timeDelta += curTime - prevTime;
    prevTime = curTime;

    DownloadStatistics tmpStats(statistics);

    tmpStats.dataCameTotalBytes += dataCame;

    // update download speed 5 times per second
    if (200 <= timeDelta)
    {
        tmpStats.downloadSpeedBytesPerSec = 1000 * dataSizeCame / timeDelta;
        if (0 < tmpStats.downloadSpeedBytesPerSec)
        {
            tmpStats.timeLeftSecs = static_cast<uint64>(dataToDownloadLeft / tmpStats.downloadSpeedBytesPerSec);
        }
        else
        {
            tmpStats.timeLeftSecs = static_cast<uint64>(DownloadStatistics::VALUE_UNKNOWN);
        }

        timeDelta = 0;
        dataSizeCame = 0;
    }

    statisticsMutex.Lock();
    statistics = tmpStats;
    statisticsMutex.Unlock();
}

DownloadStatistics Downloader::GetStatistics()
{
    LockGuard<Spinlock> lock(statisticsMutex);
    return statistics;
}

int32 Downloader::GetFileErrno() const
{
    return fileErrno;
}

int32 Downloader::GetImplError() const
{
    return implError;
}
}
