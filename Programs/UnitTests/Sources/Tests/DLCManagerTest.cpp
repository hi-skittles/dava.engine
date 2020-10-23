#include <DLCManager/DLCManager.h>
#include <FileSystem/File.h>
#include <FileSystem/FileSystem.h>
#include <DLC/Downloader/DownloadManager.h>
#include <Concurrency/Thread.h>
#include <Logger/Logger.h>
#include <Engine/Engine.h>
#include <EmbeddedWebServer/EmbeddedWebServer.h>
#include <Time/SystemTimer.h>

#include "UnitTests/UnitTests.h"

#ifndef __DAVAENGINE_WIN_UAP__

struct DownloaderTest
{
    const DAVA::DLCManager::IRequest* pack = nullptr;

    DownloaderTest()
    {
        using namespace DAVA;
        Logger::Info("before init");

        FilePath downloadedPacksDir("~doc:/UnitTests/DLCManagerTest/packs/");

        Logger::Info("clear dirs");

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

        if (!StartEmbeddedWebServer(downloadedPacksDir.GetAbsolutePathname().c_str(), "8383"))
        {
            Logger::Error("can't start embedded web server");
            TEST_VERIFY(false);
            return;
        }

        String superPackUrl("http://127.0.0.1:8383/superpack_for_unittests.dvpk");

        DLCManager& dlcManager = *GetEngineContext()->dlcManager;

        Logger::Info("init dlcManager");

        dlcManager.Initialize(downloadedPacksDir, superPackUrl, DLCManager::Hints());

        TEST_VERIFY(dlcManager.IsInitialized() == false && "need first reconnect to server check meta version");

        Logger::Info("create game client");

        dlcManager.SetRequestingEnabled(true);

        String packName = "0";

        Logger::Info("before request pack");

        pack = dlcManager.RequestPack(packName);
        TEST_VERIFY(pack != nullptr);
    }

    static bool IsInitialized()
    {
        using namespace DAVA;
        DLCManager& dlcManager = *GetEngineContext()->dlcManager;
        return dlcManager.IsInitialized();
    }

    bool IsDownloaded() const
    {
        if (pack != nullptr)
        {
            return pack->IsDownloaded();
        }
        return false;
    }
};

DAVA_TESTCLASS (DLCManagerTest)
{
    DownloaderTest downloader;

    DAVA::float32 timeLeftToInitAndDownloadPack = 40.0f; // seconds

    bool downloadOfVirtualPack = false;

    bool TestComplete(const DAVA::String& testName) const override
    {
        if (testName == "TestDownloadOfVirtualPack")
        {
            return downloadOfVirtualPack;
        }
        return true;
    }

    void Update(DAVA::float32 timeElapsed, const DAVA::String& testName) override
    {
        using namespace DAVA;
        if (testName == "TestDownloadOfVirtualPack")
        {
            if (downloader.IsInitialized())
            {
                if (downloader.IsDownloaded())
                {
                    downloadOfVirtualPack = true;
                    TEST_VERIFY(true);
                }
            }
            if (!downloadOfVirtualPack)
            {
                timeLeftToInitAndDownloadPack -= DAVA::SystemTimer::GetRealFrameDelta();
                if (timeLeftToInitAndDownloadPack < 0.f)
                {
                    downloader.IsInitialized();
                    downloader.IsDownloaded();
                    DAVA::Logger::Info("can't download pack with DLCManager");
                    TEST_VERIFY(false);
                    downloadOfVirtualPack = true; // just go to next test
                }
            }
        }
    }

    void SetUp(const DAVA::String& testName) override
    {
    }

    void TearDown(const DAVA::String& testName) override
    {
        using namespace DAVA;
        if (testName == "TestDownloadOfVirtualPack")
        {
            DAVA::StopEmbeddedWebServer();
            DLCManager& dlcManager = *DAVA::GetEngineContext()->dlcManager;
            dlcManager.Deinitialize();

            FilePath downloadedPacksDir("~doc:/UnitTests/DLCManagerTest/packs/");
            FileSystem* fs = GetEngineContext()->fileSystem;
            // every time clear directory to download once again
            fs->DeleteDirectory(downloadedPacksDir);
        }
    }

    DAVA_TEST (TestDownloadOfVirtualPack)
    {
    }
};

#endif // !__DAVAENGINE_WIN_UAP__
