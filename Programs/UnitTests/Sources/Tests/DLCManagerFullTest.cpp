#include <fstream>

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

#if !defined(__DAVAENGINE_WIN_UAP__) && !defined(__DAVAENGINE_IOS__)

DAVA::FilePath documentRootDir;
const char* const localPort = "8282";

static bool HasAccessToSystemFolder(DAVA::String dir)
{
    dir += "tmp.file";
    FILE* f = fopen(dir.c_str(), "wb");
    if (nullptr == f)
    {
        if (EACCES == errno || EROFS == errno)
        {
            return false;
        }
        DAVA::GetEngineContext()->logger->Error("Strange error code: (%d) %s for opening file: %s", errno, strerror(errno), dir.c_str());
    }

    fclose(f);
    remove(dir.c_str());
    return true;
}

struct FSMTest02
{
    enum State
    {
        WaitInitializationFinished,
        WaitSecondConnectAttempt,
        WaitDownloadAllFourPacks,
    };
    State state = WaitInitializationFinished;
    DAVA::float32 time = 0.0f;
    DAVA::float32 waitSecondConnect = 10.0f;
    const DAVA::float32 timeout = 120.f;
    DAVA::DLCManager::Progress progressAfterInit;

    void Cleanup(DAVA::DLCManager& dlcManager)
    {
        using namespace DAVA;
        dlcManager.requestUpdated.Disconnect(this);
        Logger::Info("%s Deinitialize()", __FUNCTION__);
        dlcManager.Deinitialize();
        Logger::Info("%s StopEmbeddedWebServer()", __FUNCTION__);
        StopEmbeddedWebServer();
        Logger::Info("%s done", __FUNCTION__);

        const FilePath packDir("~doc:/UnitTests/DLCManagerTest/packs/");
        GetEngineContext()->fileSystem->DeleteDirectory(packDir, true);
    }

    void OnRequestUpdateCheckOrder(const DAVA::DLCManager::IRequest& r)
    {
        static int nextDownloadedPackIndexShouldBe = 0;
        // order of downloaded pack should be "0, 1, 2, 3"
        if (r.IsDownloaded())
        {
            int packIndex = stoi(r.GetRequestedPackName());
            DVASSERT(packIndex == nextDownloadedPackIndexShouldBe);
            nextDownloadedPackIndexShouldBe += 1;
        }
    }

    bool Update(DAVA::float32 /*timeElapsed*/)
    {
        using namespace DAVA;
        DLCManager& dlcManager = *GetEngineContext()->dlcManager;

        const float32 dt = SystemTimer::GetRealFrameDelta();
        time += dt;

        switch (state)
        {
        case WaitInitializationFinished:
        {
            if (dlcManager.IsInitialized())
            {
                TEST_VERIFY(dlcManager.IsKnownFile("3d/Maps/test/images/SycamoreLeaves5.png"));
                TEST_VERIFY(dlcManager.IsKnownFile(FilePath("~res:/3d/Maps/test/images/SycamoreLeaves5.png")));
                TEST_VERIFY(dlcManager.IsKnownFile(FilePath("~res:/3d/Maps/test/images/SycamoreLeaves5.png").GetAbsolutePathname()));
                TEST_VERIFY(dlcManager.IsKnownFile(FilePath(FilePath("~res:/3d/Maps/test/images/SycamoreLeaves5.png").GetAbsolutePathname())));

                uint64 sizeOfPack = dlcManager.GetPackSize("8");

                // size from superpack_for_unittests.dvpk without any meta
                TEST_VERIFY(sizeOfPack == 29720253);

                state = WaitSecondConnectAttempt;
                StopEmbeddedWebServer();
                progressAfterInit = dlcManager.GetProgress();
                const DLCManager::IRequest* r3 = dlcManager.RequestPack("3");
                const DLCManager::IRequest* r2 = dlcManager.RequestPack("2");
                const DLCManager::IRequest* r1 = dlcManager.RequestPack("1");
                const DLCManager::IRequest* r0 = dlcManager.RequestPack("0");
                TEST_VERIFY(r3 != nullptr);
                TEST_VERIFY(r2 != nullptr);
                TEST_VERIFY(r1 != nullptr);
                TEST_VERIFY(r0 != nullptr);
                dlcManager.requestUpdated.Connect(this, &FSMTest02::OnRequestUpdateCheckOrder);

                // check reset queue after initialize finished
                dlcManager.ResetQueue();

                // do request again
                r3 = dlcManager.RequestPack("3");
                r2 = dlcManager.RequestPack("2");
                r1 = dlcManager.RequestPack("1");
                r0 = dlcManager.RequestPack("0");
                TEST_VERIFY(r3 != nullptr);
                TEST_VERIFY(r2 != nullptr);
                TEST_VERIFY(r1 != nullptr);
                TEST_VERIFY(r0 != nullptr);
                return false;
            }
        }
        break;
        case WaitSecondConnectAttempt:
        {
            TEST_VERIFY(dlcManager.IsInitialized());

            TEST_VERIFY(dlcManager.IsRequestingEnabled());

            TEST_VERIFY(dlcManager.GetInitStatus() == DLCManager::InitStatus::FinishedWithRemoteMeta);

            auto currentProgress = dlcManager.GetProgress();
            TEST_VERIFY(currentProgress.alreadyDownloaded <= currentProgress.total);

            waitSecondConnect -= dt;
            if (waitSecondConnect <= 0.f)
            {
                if (!StartEmbeddedWebServer(documentRootDir.GetAbsolutePathname().c_str(), localPort))
                {
                    TEST_VERIFY(false && "can't start server");
                }
                state = WaitDownloadAllFourPacks;
                return false;
            }
        }
        break;
        case WaitDownloadAllFourPacks:
        {
            auto currentProgress = dlcManager.GetProgress();
            TEST_VERIFY(currentProgress.alreadyDownloaded <= currentProgress.total);
            progressAfterInit = currentProgress;

            if (!dlcManager.IsAnyPackInQueue())
            {
                auto r0 = dlcManager.RequestPack("0");
                TEST_VERIFY(r0->IsDownloaded());
                auto r1 = dlcManager.RequestPack("1");
                TEST_VERIFY(r1->IsDownloaded());
                auto r2 = dlcManager.RequestPack("2");
                TEST_VERIFY(r2->IsDownloaded());
                auto r3 = dlcManager.RequestPack("3");
                TEST_VERIFY(r3->IsDownloaded());

                DLCManager::Progress progressForPack = dlcManager.GetPacksProgress({ "3" }); // 3 depends on 2, on 1, on 0
                DLCManager::Progress progressAll = dlcManager.GetProgress();
                // now progress should match
                TEST_VERIFY(progressForPack.total < progressAll.total);
                TEST_VERIFY(progressForPack.total == progressAll.alreadyDownloaded);
                TEST_VERIFY(progressForPack.alreadyDownloaded == progressForPack.total); // all downloaded value = 13669086
                TEST_VERIFY(progressForPack.total == 13669086); // check it out

                // now stop server for next tests
                Cleanup(dlcManager);
                return true;
            }

            DLCManager::Progress progress0 = dlcManager.GetPacksProgress({ "0" });
            DLCManager::Progress progress1 = dlcManager.GetPacksProgress({ "1" });
            DLCManager::Progress progress2 = dlcManager.GetPacksProgress({ "2" });
            DLCManager::Progress progress3 = dlcManager.GetPacksProgress({ "3" });
            DLCManager::Progress progress4 = dlcManager.GetPacksProgress({ "4" });
            DLCManager::Progress progress5 = dlcManager.GetPacksProgress({ "5" });
            DLCManager::Progress progress6 = dlcManager.GetPacksProgress({ "6" });
            DLCManager::Progress progress7 = dlcManager.GetPacksProgress({ "7" });
            DLCManager::Progress progress8 = dlcManager.GetPacksProgress({ "8" });
            DLCManager::Progress progressAll = dlcManager.GetProgress();
            TEST_VERIFY(progress0.total < progress1.total);
            TEST_VERIFY(progress1.total < progress2.total);
            TEST_VERIFY(progress2.total < progress3.total);
            TEST_VERIFY(progress3.total < progress4.total);
            TEST_VERIFY(progress4.total < progress5.total);
            TEST_VERIFY(progress5.total < progress6.total);
            TEST_VERIFY(progress6.total < progress7.total);
            TEST_VERIFY(progress7.total < progress8.total);
            TEST_VERIFY(progress8.total == progressAll.total);
        }
        break;
        }

        if (time > timeout)
        {
            auto prog = dlcManager.GetProgress();

            Logger::Error("time > timeout (%f > %f)", time, timeout);
            Logger::Error("timeout: total: %llu downloaded: %lld", prog.total, prog.alreadyDownloaded);

            FilePath logPath(DLCManager::Hints().logFilePath);
            String path = logPath.GetAbsolutePathname();
            std::ifstream dlcLogFile(path.c_str());

            if (!dlcLogFile)
            {
                Logger::Error("can't open dlcManager.log file at: %s", path.c_str());
            }
            else
            {
                Logger::Error("begin-------dlcManager.log---------content");
                for (String str; getline(dlcLogFile, str);)
                {
                    Logger::Error("%s", str.c_str());
                }
                Logger::Error("end-------dlcManager.log---------content");
            }

            Cleanup(dlcManager);

            TEST_VERIFY(false && "time out wait second connection");
            return true;
        }

        return false;
    }
};

DAVA_TESTCLASS (DLCManagerFullTest)
{
    FSMTest02 fsm02;
    bool TestAfterInitStopServer02_done = false;

    bool TestComplete(const DAVA::String& testName) const override
    {
        if (testName == "TestAfterInitStopServer02")
        {
            return TestAfterInitStopServer02_done;
        }
        return true;
    }

    void Update(DAVA::float32 timeElapsed, const DAVA::String& testName) override
    {
        if (testName == "TestAfterInitStopServer02")
        {
            TestAfterInitStopServer02_done = fsm02.Update(timeElapsed);
        }
    }

    DAVA_TEST (TestInitOnlyLocalMeta)
    {
        using namespace DAVA;

        DLCManager& dlcManager = *GetEngineContext()->dlcManager;
        try
        {
            DLCManager::Hints hints;

            hints.localPacksDB = "~doc:/local_fake_meta.db";
            FileSystem* fs = GetEngineContext()->fileSystem;
            fs->CopyFile("~res:/TestData/DLCManagerFullTest/local_fake_meta.db",
                         "~doc:/local_fake_meta.db", true);
            dlcManager.Initialize("~doc:/packs/", "", hints);
        }
        catch (std::exception& ex)
        {
            Logger::Error("can't initialize DLCManager without remote url, exception: %s", ex.what());
            TEST_VERIFY(false && "failed init dlc_manager without url");
        }

        TEST_VERIFY(dlcManager.IsKnownFile("some/not/existing/path"));
        TEST_VERIFY(dlcManager.IsKnownFile(FilePath("some/not/existing/path")));
        TEST_VERIFY(dlcManager.IsKnownFile(FilePath("~res:/some/not/existing/path")));

        dlcManager.Deinitialize();
    }

    DAVA_TEST (TestInitOnlyRemoteMeta)
    {
        using namespace DAVA;

        DLCManager& dlcManager = *GetEngineContext()->dlcManager;
        try
        {
            const DLCManager::Hints hints;
            char fullUrl[1024] = { 0 };
            sprintf(fullUrl, "http://127.0.0.1:%s/superpack_for_unittests.dvpk", localPort);
            dlcManager.Initialize("~doc:/packs/", fullUrl, hints);
        }
        catch (std::exception& ex)
        {
            Logger::Error("can't initialize DLCManager without remote url, exception: %s", ex.what());
            TEST_VERIFY(false && "failed init dlc_manager without url");
        }

        dlcManager.Deinitialize();
    }

    DAVA_TEST (TestInitializeBadFolder01)
    {
        using namespace DAVA;

        DLCManager& dlcManager = *GetEngineContext()->dlcManager;

        bool getException = false;

#ifdef __DAVAENGINE_WINDOWS__
        const char* cant_write_dir = "C:/Windows/"; // system dir
#else
        const char* cant_write_dir = "/"; // root dir
#endif
        if (HasAccessToSystemFolder(cant_write_dir))
        {
            return; // we run as root, just skip this test
        }

        try
        {
            char fullUrl[1024] = { 0 };
            sprintf(fullUrl, "http://127.0.0.1:%s/superpack_for_unittests.dvpk", localPort);

            dlcManager.Initialize(cant_write_dir, fullUrl, DLCManager::Hints());
        }
        catch (Exception& ex)
        {
            Logger::Info("get known exception: %s", ex.what());
            getException = true;
        }

        TEST_VERIFY(getException && "can't write or no such folder exception missing");

        dlcManager.Deinitialize();
    }

    DAVA_TEST (TestAfterInitStopServer02)
    {
        using namespace DAVA;

        documentRootDir = "~doc:/";

        Logger::Info("First part of TestAfterInitStopServer02 started");

        DLCManager& dlcManager = *GetEngineContext()->dlcManager;

        const DLCManager::IRequest* r = dlcManager.RequestPack("1"); // pack "1" have one dependent pack "0"
        TEST_VERIFY(r != nullptr);

        FileSystem* fs = GetEngineContext()->fileSystem;

        const FilePath destPath = documentRootDir + "superpack_for_unittests.dvpk";
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

        if (!StartEmbeddedWebServer(documentRootDir.GetAbsolutePathname().c_str(), localPort))
        {
            TEST_VERIFY(false && "can't start embedded web server");
            return;
        }

        auto hints = DLCManager::Hints();
        hints.retryConnectMilliseconds = 3000;

        const FilePath packDir("~doc:/UnitTests/DLCManagerTest/packs/");
        fs->DeleteDirectory(packDir, true);

        try
        {
            char fullUrl[1024] = { 0 };
            sprintf(fullUrl, "http://127.0.0.1:%s/superpack_for_unittests.dvpk", localPort);

            FilePath dbPath("~res:/TestData/DLCManagerFullTest/local_fake_meta.db");
            TEST_VERIFY(fs->IsFile(dbPath) == true);

            FileSystem* fs = GetEngineContext()->fileSystem;
            FilePath docDbPath("~doc:/local_fake_meta.db");
            fs->CopyFile(dbPath, docDbPath, true);

            hints.localPacksDB = docDbPath.GetStringValue();

            dlcManager.Initialize(packDir,
                                  fullUrl,
                                  hints);

            Logger::Info("Initialize called no exception");

            TEST_VERIFY(true == dlcManager.IsRequestingEnabled());

            const String pack1("fake_pack_00");
            const DLCManager::IRequest* request1 = dlcManager.RequestPack(pack1);
            TEST_VERIFY(request1 != nullptr);
            TEST_VERIFY(request1->GetRequestedPackName() == pack1);
            TEST_VERIFY(request1->IsDownloaded());

            const String pack2("fake_pack_01");
            const DLCManager::IRequest* request2 = dlcManager.RequestPack(pack2);
            TEST_VERIFY(request2 != nullptr);
            TEST_VERIFY(request2->GetRequestedPackName() == pack2);
            TEST_VERIFY(request2->IsDownloaded());

            // check reset queue before initialize finished
            dlcManager.ResetQueue();

            // check after reset queue
            request1 = dlcManager.RequestPack(pack1);
            TEST_VERIFY(request1 != nullptr);
            TEST_VERIFY(request1->GetRequestedPackName() == pack1);
            TEST_VERIFY(request1->IsDownloaded());

            request2 = dlcManager.RequestPack(pack2);
            TEST_VERIFY(request2 != nullptr);
            TEST_VERIFY(request2->GetRequestedPackName() == pack2);
            TEST_VERIFY(request2->IsDownloaded());
        }
        catch (std::exception& ex)
        {
            Logger::Error("error: %s", ex.what());
            TEST_VERIFY(false && "can't initialize dlcManager");
        }

        auto request = dlcManager.RequestPack("3"); // pack "3" depends on "0, 1, 2" packs
        TEST_VERIFY(request != nullptr);
        TEST_VERIFY(dlcManager.IsRequestingEnabled());
        Logger::Info("First part of TestAfterInitStopServer02 finished");
    }
};

#endif // __DAVAENGINE_WIN_UAP__
