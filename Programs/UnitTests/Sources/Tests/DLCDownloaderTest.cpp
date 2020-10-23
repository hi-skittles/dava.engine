#include "UnitTests/UnitTests.h"

#if !defined(__DAVAENGINE_WIN_UAP__) && !defined(__DAVAENGINE_IOS__)
#include <DLCManager/DLCDownloader.h>
#include <DLCManager/Private/DLCManagerImpl.h>
#include <Logger/Logger.h>
#include <DLC/Downloader/DownloadManager.h>
#include <FileSystem/FileSystem.h>
#include <Time/SystemTimer.h>
#include <Utils/CRC32.h>
#include <EmbeddedWebServer/EmbeddedWebServer.h>
#include <Engine/Engine.h>

#include <iomanip>

#include <EmbeddedWebServer/Private/mongoose.h>

static const DAVA::String URL = "http://127.0.0.1:8181/superpack_for_unittests.dvpk";
// "http://127.0.0.1:8080/superpack_for_unittests.dvpk"; // embedded web server
// "http://dl-wotblitz.wargaming.net/dlc/r11608713/3.7.0.236.dvpk"; // CDN
// "http://by1-builddlc-01.corp.wargaming.local/DLC_Blitz/smart_dlc/3.7.0.236.dvpk" // local net server

class EmbededWebServer
{
public:
    static volatile bool allwaysReturnErrorStaticHtml;
    static DAVA::int32 OnHttpRequestHandler(mg_connection* conn)
    {
        using namespace DAVA;
        if (allwaysReturnErrorStaticHtml)
        {
            const char* content = "server return more data then we ask! (we ask last 4 bytes) buffer overflow check";
            const char* mimeType = "text/plain";
            const int contentLength = static_cast<int>(strlen(content));

            mg_printf(conn,
                      "HTTP/1.1 200 OK\r\n"
                      "Cache: no-cache\r\n"
                      "Content-Type: %s\r\n"
                      "Content-Length: %d\r\n"
                      "\r\n",
                      mimeType,
                      contentLength);
            mg_write(conn, content, contentLength);
            return 1;
        }
        return 0; // mongoose will handle request
    }
    EmbededWebServer()
    {
        using namespace DAVA;
        FilePath downloadedPacksDir("~doc:/UnitTests/DLCManagerTest/packs/");

        FileSystem* fs = GetEngineContext()->fileSystem;
        // every time clear directory to download once again
        fs->DeleteDirectory(downloadedPacksDir);
        fs->CreateDirectory(downloadedPacksDir, true);

        const FilePath destPath = downloadedPacksDir + "superpack_for_unittests.dvpk";
        const FilePath srcPath = "~res:/TestData/DLCManagerFullTest/superpack_for_unittests.dvpk";
        if (!fs->IsFile(srcPath))
        {
            Logger::Error("no super pack file!");
            TEST_VERIFY(false);
        }

        if (!fs->CopyFile(srcPath, destPath, true))
        {
            Logger::Error("can't copy super pack for unittest from res:/");
            TEST_VERIFY(false);
            return;
        }

        String path = downloadedPacksDir.GetAbsolutePathname();

        if (!StartEmbeddedWebServer(path.c_str(), "8181", &OnHttpRequestHandler))
        {
            DAVA_THROW(DAVA::Exception, "can't start embedded web server");
        }
    }
    ~EmbededWebServer()
    {
        DAVA::StopEmbeddedWebServer();
    }
};

volatile bool EmbededWebServer::allwaysReturnErrorStaticHtml = false;

DAVA_TESTCLASS (DLCDownloaderTest)
{
    EmbededWebServer embeddedServer;
    const DAVA::int64 FULL_SIZE_ON_SERVER = 29738138; // old full dlc build size 1618083461;

    DAVA_TEST (GetFileSizeTest)
    {
        using namespace DAVA;

        DLCDownloader* downloader = DLCDownloader::Create();
        String url = URL;
        DLCDownloader::ITask* task = downloader->StartGetContentSize(url);

        downloader->WaitTask(task);

        auto& info = downloader->GetTaskInfo(task);
        auto& status = downloader->GetTaskStatus(task);

        TEST_VERIFY(info.rangeOffset == -1);
        TEST_VERIFY(info.rangeSize == -1);
        TEST_VERIFY(info.dstPath == "");
        TEST_VERIFY(info.srcUrl == url);
        TEST_VERIFY(info.timeoutSec >= 0);
        TEST_VERIFY(info.type == DLCDownloader::TaskType::SIZE);

        TEST_VERIFY(status.error.httpCode == 200);
        TEST_VERIFY(status.error.errorHappened == false);
        TEST_VERIFY(status.error.curlErr == 0);
        TEST_VERIFY(status.error.errStr == String(""));
        TEST_VERIFY(status.error.curlMErr == 0);
        TEST_VERIFY(status.error.fileErrno == 0);
        TEST_VERIFY(status.sizeDownloaded == 0);
        TEST_VERIFY(status.state.load() == DLCDownloader::TaskState::Finished);
        TEST_VERIFY(status.sizeTotal == FULL_SIZE_ON_SERVER);

        downloader->RemoveTask(task);

        DLCDownloader::Destroy(downloader);
    }

    DAVA_TEST (RangeRequestTest)
    {
        using namespace DAVA;

        std::array<char, 4> buf;
        std::shared_ptr<MemoryBufferWriter> writer = std::make_shared<MemoryBufferWriter>(buf.data(), buf.size());

        DLCDownloader* downloader = DLCDownloader::Create();
        String url = URL;
        int64 startRangeIndex = FULL_SIZE_ON_SERVER - 4;
        int64 rangeSize = 4;
        DLCDownloader::ITask* downloadLast4Bytes = downloader->StartTask(url, writer, DLCDownloader::Range(startRangeIndex, rangeSize));

        downloader->WaitTask(downloadLast4Bytes);

        auto& info = downloader->GetTaskInfo(downloadLast4Bytes);
        auto& status = downloader->GetTaskStatus(downloadLast4Bytes);

        TEST_VERIFY(info.rangeOffset == startRangeIndex);
        TEST_VERIFY(info.rangeSize == rangeSize);
        TEST_VERIFY(info.dstPath == "");
        TEST_VERIFY(info.srcUrl == url);
        TEST_VERIFY(info.timeoutSec >= 0);
        TEST_VERIFY(info.type == DLCDownloader::TaskType::FULL);

        TEST_VERIFY(status.error.errorHappened == false);
        TEST_VERIFY(status.error.httpCode <= 206);
        TEST_VERIFY(status.error.curlErr == 0);
        TEST_VERIFY(status.error.errStr == String(""));
        TEST_VERIFY(status.error.curlMErr == 0);
        TEST_VERIFY(status.error.fileErrno == 0);
        TEST_VERIFY(status.sizeDownloaded == 4);
        TEST_VERIFY(status.state == DLCDownloader::TaskState::Finished);
        TEST_VERIFY(status.sizeTotal == 4);

        std::array<char, 4> shouldBe{ 'D', 'V', 'P', 'K' };
        TEST_VERIFY(shouldBe == buf);

        downloader->RemoveTask(downloadLast4Bytes);

        DLCDownloader::Destroy(downloader);
    }

    DAVA_TEST (EmptyRangeRequestTest)
    {
        using namespace DAVA;

        // Implementation of std::array<T,0>::data() in msvc2017 returns nullptr which leads
        // to assert inside MemoryBufferWriter constructor
        std::array<char, 1> buf = { 0 };
        std::shared_ptr<MemoryBufferWriter> writer = std::make_shared<MemoryBufferWriter>(buf.data(), 0);

        DLCDownloader* downloader = DLCDownloader::Create();
        String url = URL;
        int64 startRangeIndex = FULL_SIZE_ON_SERVER - 4;
        int64 rangeSize = 0;
        DLCDownloader::ITask* downloadLast0Bytes = downloader->StartTask(url, writer, DLCDownloader::Range(startRangeIndex, rangeSize));

        downloader->WaitTask(downloadLast0Bytes);

        auto& info = downloader->GetTaskInfo(downloadLast0Bytes);
        auto& status = downloader->GetTaskStatus(downloadLast0Bytes);

        TEST_VERIFY(info.rangeOffset == startRangeIndex);
        TEST_VERIFY(info.rangeSize == rangeSize);
        TEST_VERIFY(info.dstPath == "");
        TEST_VERIFY(info.srcUrl == url);
        TEST_VERIFY(info.timeoutSec >= 0);
        TEST_VERIFY(info.type == DLCDownloader::TaskType::FULL);

        TEST_VERIFY(status.error.errorHappened == false);
        TEST_VERIFY(status.error.httpCode <= 206);
        TEST_VERIFY(status.error.curlErr == 0);
        TEST_VERIFY(status.error.errStr == String(""));
        TEST_VERIFY(status.error.curlMErr == 0);
        TEST_VERIFY(status.error.fileErrno == 0);
        TEST_VERIFY(status.sizeDownloaded == 0);
        TEST_VERIFY(status.state == DLCDownloader::TaskState::Finished);
        TEST_VERIFY(status.sizeTotal == 0);

        std::array<char, 1> shouldBe = { 0 };
        TEST_VERIFY(shouldBe == buf);

        downloader->RemoveTask(downloadLast0Bytes);

        DLCDownloader::Destroy(downloader);
    }

    DAVA_TEST (DowloadLargeFileTest)
    {
        using namespace DAVA;

        FileSystem* fs = FileSystem::Instance();
        std::unique_ptr<DLCDownloader> downloader(DLCDownloader::Create());
        String url = URL;

        FilePath path("~doc:/big_tmp_file_from_server.remove.me");
        const bool file_removed = fs->DeleteFile(path);

        String p = path.GetAbsolutePathname();
        int64 start = 0;
        DLCDownloader::ITask* task = nullptr;
        int64 finish = 0;
        float64 seconds = 0.0;
        float64 sizeInGb = FULL_SIZE_ON_SERVER / (1024.0 * 1024.0 * 1024.0);

        //// ----next-------------------------------------------------------
        {
            start = SystemTimer::GetMs();

            task = downloader->StartTask(url, p);

            downloader->WaitTask(task);
            downloader->RemoveTask(task);
        }

        finish = SystemTimer::GetMs();

        seconds = (finish - start) / 1000.0;

        Logger::Info("new downloader %f Gb download from in house server for: %f", sizeInGb, seconds);

        const uint32 crc32 = 0x89D4BC4E; // old crc32 for full build 0xDE5C2B62;

        uint32 crcFromFile = CRC32::ForFile(p);

        TEST_VERIFY(crcFromFile == crc32);

        ////-----resume-downloading------------------------------------------------
        File* file = File::Create(p, File::OPEN | File::READ | File::WRITE);
        if (file)
        {
            uint64 fileSize = file->GetSize();
            TEST_VERIFY(fileSize == FULL_SIZE_ON_SERVER);

            bool result = file->Truncate(fileSize / 2);
            TEST_VERIFY(result);

            file->Release();
        }
        task = downloader->ResumeTask(url, p);

        downloader->WaitTask(task);

        downloader->RemoveTask(task);

        crcFromFile = CRC32::ForFile(p);
        TEST_VERIFY(crcFromFile == crc32);

        ////-----multi-files-------------------------------------------------------
        DLCDownloader::ITask* taskSize = downloader->StartGetContentSize(url);
        downloader->WaitTask(taskSize);
        uint64 sizeTotal = downloader->GetTaskStatus(taskSize).sizeTotal;

        uint64 firstIndex = 0;
        uint64 nextIndex = 0;
        //const uint64 lastIndex = sizeTotal - 1;

        FilePath dir("~doc:/multy_tmp/");
        fs->DeleteDirectory(dir, true);
        fs->CreateDirectory(dir);

        const size_t numAll = 1024;
        const size_t onePart = static_cast<size_t>(sizeTotal / numAll);

        nextIndex += onePart;

        Vector<DLCDownloader::ITask*> allTasks;
        allTasks.reserve(numAll);

        start = SystemTimer::GetMs();

        for (size_t i = 0; i < numAll; ++i, firstIndex += onePart, nextIndex += onePart)
        {
            StringStream ss;
            ss << "part_" << std::setw(5) << std::setfill('0') << i << '_' << firstIndex << '-' << nextIndex;
            String fileName = ss.str() + ".part";
            FilePath pathFull = dir + fileName;
            String full = pathFull.GetAbsolutePathname();
            task = downloader->StartTask(url, full, DLCDownloader::Range(firstIndex, nextIndex - firstIndex));
            allTasks.push_back(task);
        }

        for (auto t : allTasks)
        {
            downloader->WaitTask(t);
        }

        finish = SystemTimer::GetMs();

        seconds = (finish - start) / 1000.0;

        Logger::Info("%d part of %f Gb download from in house server for: %f", static_cast<int>(numAll), sizeInGb, seconds);

        // free memory
        for (auto t : allTasks)
        {
            downloader->RemoveTask(t);
        }
        allTasks.clear();
    }

    DAVA_TEST (ISP_return_internalErrorPageTest)
    {
        using namespace DAVA;

        EmbededWebServer::allwaysReturnErrorStaticHtml = true;

        {
            Logger::Info("just before start test errno: %d %s", errno, strerror(errno));

            std::array<char, 5> buf{ 'c', 'l', 'e', 'a', 'r' };
            std::shared_ptr<MemoryBufferWriter> writer = std::make_shared<MemoryBufferWriter>(buf.data(), buf.size());

            DLCDownloader* downloader = DLCDownloader::Create();
            String url = URL;
            int64 startRangeIndex = FULL_SIZE_ON_SERVER - 4;
            int64 rangeSize = 4;
            DLCDownloader::ITask* downloadLast4Bytes = downloader->StartTask(url, writer, DLCDownloader::Range(startRangeIndex, rangeSize));

            downloader->WaitTask(downloadLast4Bytes);

            auto& info = downloader->GetTaskInfo(downloadLast4Bytes);
            auto& status = downloader->GetTaskStatus(downloadLast4Bytes);

            TEST_VERIFY(info.rangeOffset == startRangeIndex);
            TEST_VERIFY(info.rangeSize == rangeSize);
            TEST_VERIFY(info.dstPath == "");
            TEST_VERIFY(info.srcUrl == url);
            TEST_VERIFY(info.timeoutSec >= 0);
            TEST_VERIFY(info.type == DLCDownloader::TaskType::FULL);

            TEST_VERIFY(status.error.errorHappened == true);
            TEST_VERIFY(status.error.httpCode <= 206);
            TEST_VERIFY(status.error.curlErr == 23); // error write
            TEST_VERIFY(status.error.errStr != String(""));
            TEST_VERIFY(status.error.curlMErr == 0);
            if (status.error.fileErrno != 0)
            {
                Logger::Info("why is it so? errno: %d %s", status.error.fileErrno, strerror(status.error.fileErrno));
            }
            TEST_VERIFY(status.sizeDownloaded == 0);
            TEST_VERIFY(status.state == DLCDownloader::TaskState::Finished);
            TEST_VERIFY(status.sizeTotal == 4);

            // We want downloader do not touch output buffer as soon as
            // error happened. So our buffer should be same.

            std::array<char, 5> shouldBe{ 'c', 'l', 'e', 'a', 'r' }; // first part
            TEST_VERIFY(shouldBe == buf);

            downloader->RemoveTask(downloadLast4Bytes);

            DLCDownloader::Destroy(downloader);
        }

        EmbededWebServer::allwaysReturnErrorStaticHtml = false;
    }
};
#endif
