#include <DLCManager/DLCManager.h>
#include <DLCManager/DLCDownloader.h>
#include <FileSystem/File.h>
#include <FileSystem/FileList.h>
#include <FileSystem/FileSystem.h>
#include <Concurrency/Thread.h>
#include <Logger/Logger.h>
#include <Engine/Engine.h>
#include <EmbeddedWebServer/EmbeddedWebServer.h>
#include <Time/SystemTimer.h>

#include "UnitTests/UnitTests.h"

#ifndef __DAVAENGINE_WIN_UAP__

static const DAVA::String superPackUrl("http://127.0.0.1:8383/superpack_for_unittests.dvpk");
static const DAVA::String wwwDirStr("~doc:/UnitTests/DLCManagerTest/local_www/");
static const DAVA::String dlcDirTemplate("~doc:/UnitTests/DLCManagerTest/dlc_dir_");
static const DAVA::String dlcLogTemplate("~doc:/wrap_dlc_");
static const int numDlcWrapers = 3; // use not more 8 like packs in superpack_for_unittests.dvpk
static const bool useOneDownloaderForAllDlcWrappers = true;

struct DLCWrapper
{
    DAVA::DLCManager& dlcManager;
    DAVA::String downloadDir;
    std::shared_ptr<DAVA::DLCDownloader> downloader;
    DAVA::String packName;

    DLCWrapper(DAVA::DLCManager& dlcManager_,
               const DAVA::String& downloadDir_,
               const DAVA::String& logFilePath,
               const DAVA::String& packName_,
               std::shared_ptr<DAVA::DLCDownloader> downloader_)
        : dlcManager(dlcManager_)
        , downloadDir(downloadDir_)
        , downloader(downloader_)
        , packName(packName_)
    {
        using namespace DAVA;
        FilePath downloadedPacksDir(downloadDir);

        auto hints = DLCManager::Hints();
        hints.logFilePath = logFilePath;
        hints.downloader = downloader;

        dlcManager.Initialize(downloadedPacksDir, superPackUrl, hints);

        TEST_VERIFY(dlcManager.IsInitialized() == false && "need first reconnect to server check meta version");

        dlcManager.SetRequestingEnabled(true);

        const DAVA::DLCManager::IRequest* pack = dlcManager.RequestPack(packName);
        TEST_VERIFY(pack != nullptr);
    }
    ~DLCWrapper()
    {
        using namespace DAVA;
        dlcManager.Deinitialize();

        FileSystem* fs = GetEngineContext()->fileSystem;
        // every time clear directory to download once again
        FilePath dir(downloadDir);
        fs->DeleteDirectory(dir);

        if (&dlcManager != DAVA::GetEngineContext()->dlcManager)
        {
            DLCManager::Destroy(&dlcManager);
        }
    }

    bool IsInitialized()
    {
        return dlcManager.IsInitialized();
    }

    bool IsDownloaded() const
    {
        if (auto pack = dlcManager.RequestPack(packName))
        {
            return pack->IsDownloaded();
        }
        return false;
    }
};

DAVA_TESTCLASS (DLCManagerMultiTest)
{
    std::vector<std::unique_ptr<DLCWrapper>> dlcWrappers;

    DAVA::float32 timeLeftToInitAndDownloadPack = 40.f; // seconds

    DAVA::DLCManager* dlcManagerCustom = nullptr;

    bool TestComplete(const DAVA::String& testName) const override
    {
        if (testName == "TestTwoDLCManagersRequestingSamePack")
        {
            auto IsPackDownloaded = [](const std::unique_ptr<DLCWrapper>& dlc) {
                return dlc->IsDownloaded();
            };
            bool allDownloaded = std::all_of(begin(dlcWrappers), end(dlcWrappers), IsPackDownloaded);
            return allDownloaded || timeLeftToInitAndDownloadPack <= 0.f;
        }
        return true;
    }

    void Update(DAVA::float32 timeElapsed, const DAVA::String& testName) override
    {
        if (testName == "TestTwoDLCManagersRequestingSamePack")
        {
            timeLeftToInitAndDownloadPack -= DAVA::SystemTimer::GetRealFrameDelta();
            TEST_VERIFY(timeLeftToInitAndDownloadPack > 0.f);
        }
    }

    DAVA_TEST (TestTwoDLCManagersRequestingSamePack)
    {
        using namespace DAVA;
        if (dlcWrappers.empty())
        {
            FileSystem* fs = GetEngineContext()->fileSystem;
            // every time clear directory to download once again
            FilePath wwwDir(wwwDirStr);
            fs->DeleteDirectory(wwwDir);
            fs->CreateDirectory(wwwDir, true);

            const FilePath destPath = wwwDir + "superpack_for_unittests.dvpk";
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

            bool webServerStarted = StartEmbeddedWebServer(wwwDir.GetAbsolutePathname().c_str(), "8383");
            TEST_VERIFY(webServerStarted);
            if (!webServerStarted)
            {
                Logger::Error("can't start embedded web server");
                timeLeftToInitAndDownloadPack = 0.f; // just finish test
            }

            std::shared_ptr<DLCDownloader> commonDownloader = useOneDownloaderForAllDlcWrappers ?
            std::shared_ptr<DLCDownloader>(DLCDownloader::Create(), DLCDownloader::Destroy) :
            std::shared_ptr<DLCDownloader>();

            for (int i = 0; i < numDlcWrapers; ++i)
            {
                DLCManager* dlc = i == 0 ? DAVA::GetEngineContext()->dlcManager : DLCManager::Create();
                String dlcDir = dlcDirTemplate + std::to_string(i) + "/";
                String dlcLog = dlcLogTemplate + std::to_string(i) + "_log.txt";
                String packNameToDownload = std::to_string(i);
                dlcWrappers.push_back(std::make_unique<DLCWrapper>(*dlc, dlcDir, dlcLog, packNameToDownload, commonDownloader));
            }
        }
    }

    DAVA_TEST (TestTwoDLCManagersRequestingSamePack_CheckDirs)
    {
        using namespace DAVA;

        for (auto& dlc : dlcWrappers)
        {
            ScopedPtr<FileList> fileList1(new FileList(dlcWrappers.front()->downloadDir));
            ScopedPtr<FileList> fileList2(new FileList(dlc->downloadDir));

            // pack dependency matrix for superpack_for_unittests.dvpk
            // so we know every N+1 pack have all previouse files from N pack

            // START - DEPENDENCY
            // pack - index | num - of - dependencies | all - dependency - indexes |
            // ---------------------------------------------------------- -
            // p0 | 0 |
            // p1 | 1 | p0
            // p2 | 2 | p1 p0
            // p3 | 3 | p2 p1 p0
            // p4 | 4 | p3 p2 p1 p0
            // p5 | 5 | p4 p3 p2 p1 p0
            // p6 | 6 | p5 p4 p3 p2 p1 p0
            // p7 | 7 | p6 p5 p4 p3 p2 p1 p0
            // p8 | 8 | p7 p6 p5 p4 p3 p2 p1 p0
            // - END - DEPENDENCY------------------------------------------------

            TEST_VERIFY(fileList1->GetFileCount() >= 3); // we know at least should be 3 files:
            // local_copy_server_file_table.block, local_copy_server_footer.footer, local_copy_server_meta.meta
            TEST_VERIFY(fileList1->GetFileCount() <= fileList2->GetFileCount());

            TEST_VERIFY(fileList1->GetDirectoryCount() > 0);
            TEST_VERIFY(fileList1->GetDirectoryCount() <= fileList2->GetDirectoryCount());
        }
    }

    DAVA_TEST (TestTwoDLCManagersRequestingSamePack_StopServer)
    {
        using namespace DAVA;
        StopEmbeddedWebServer();

        FileSystem* fs = GetEngineContext()->fileSystem;
        // every time clear directory to download once again
        FilePath wwwDir(wwwDirStr);
        fs->DeleteDirectory(wwwDir);

        dlcWrappers.clear();
    }
};

#endif // !__DAVAENGINE_WIN_UAP__
