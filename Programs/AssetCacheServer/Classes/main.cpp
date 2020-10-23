#include "UI/AssetCacheServerWindow.h"
#include "ServerCore.h"
#include "Logger/RotationLogger.h"

#include <QtHelpers/RunGuard.h>
#include <QtHelpers/LauncherListener.h>
#include <QtHelpers/CrashDumpHandler.h>

#include <DocDirSetup/DocDirSetup.h>

#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <Logger/Logger.h>
#include <FileSystem/FileSystem.h>
#include <Debug/DVAssertDefaultHandlers.h>

#include <QApplication>
#include <QCryptographicHash>

int Process(DAVA::Engine& e)
{
    using namespace DAVA;

    const EngineContext* context = e.GetContext();
    context->logger->SetLogLevel(DAVA::Logger::LEVEL_FRAMEWORK);

    RotationLogger fullLogger(context);
    fullLogger.SetLogPath("~doc:/AssetCacheServerLogs/full.log");
    fullLogger.SetLogLevel(DAVA::Logger::LEVEL_FRAMEWORK);

    RotationLogger alertLogger(context);
    alertLogger.SetLogPath("~doc:/AssetCacheServerLogs/alert.log");
    alertLogger.SetLogLevel(DAVA::Logger::LEVEL_INFO);

    const QString appUid = "{DAVA.AssetCacheServer.Version.1.0.0}";
    const QString appUidPath = QCryptographicHash::hash((appUid).toUtf8(), QCryptographicHash::Sha1).toHex();
    std::unique_ptr<QtHelpers::RunGuard> runGuard = std::make_unique<QtHelpers::RunGuard>(appUidPath);
    if (!runGuard->TryToRun())
    {
        Logger::Warning("Can't start: application is already running");
        return -1;
    }

    Vector<char*> argv = e.GetCommandLineAsArgv();
    int argc = static_cast<int>(argv.size());

    QApplication a(argc, argv.data());

    DAVA::FileSystem* fs = context->fileSystem;

#ifdef __DAVAENGINE_MACOS__
    auto copyDocumentsFromOldFolder = [&]
    {
        FileSystem::eCreateDirectoryResult createResult = DocumentsDirectorySetup::CreateApplicationDocDirectory(fs, "AssetServer");
        if (createResult != DAVA::FileSystem::DIRECTORY_EXISTS)
        {
            ApplicationSettings settings;
            settings.LoadFromOldPath();
            FilePath cacheContentsPath = settings.GetFolder();
            cacheContentsPath.MakeDirectoryPathname();

            FilePath documentsFolderOld = fs->GetCurrentDocumentsDirectory();
            DocumentsDirectorySetup::SetApplicationDocDirectory(fs, "AssetServer");
            if (cacheContentsPath.StartsWith(documentsFolderOld))
            {
                FilePath cacheContentsDefaultPath = settings.GetDefaultFolder();
                cacheContentsDefaultPath.MakeDirectoryPathname();
                if (fs->CreateDirectory(cacheContentsDefaultPath, true) == FileSystem::DIRECTORY_CREATED)
                {
                    fs->RecursiveCopy(cacheContentsPath, cacheContentsDefaultPath);
                    settings.SetFolder(cacheContentsDefaultPath);
                }
            }

            settings.Save(); // settings are saved in new place
        }
    };
    copyDocumentsFromOldFolder(); // todo: remove some versions after
#endif
    DocumentsDirectorySetup::SetApplicationDocDirectory(fs, "AssetServer");

    std::unique_ptr<ServerCore> server = std::make_unique<ServerCore>();
    server->SetApplicationPath(QApplication::applicationFilePath().toStdString());
    std::unique_ptr<AssetCacheServerWindow> mainWindow = std::make_unique<AssetCacheServerWindow>(*server);

    if (server->Settings().IsFirstLaunch())
    {
        mainWindow->OnFirstLaunch();
    }

    if (server->Settings().IsAutoStart())
    {
        server->Start();
    }

    QObject::connect(&a, &QApplication::aboutToQuit, [&]()
                     {
                         mainWindow.reset();
                         server.reset();
                         runGuard.reset();
                     });

    LauncherListener launcherListener;
    launcherListener.Init([](LauncherListener::eMessage message) {
        if (message == LauncherListener::eMessage::QUIT)
        {
            Logger::Info("Got quit message from launcher listener");
            qApp->quit();
            return LauncherListener::eReply::ACCEPT;
        }
        return LauncherListener::eReply::UNKNOWN_MESSAGE;
    });

    return a.exec();
}

int DAVAMain(DAVA::Vector<DAVA::String> cmdLine)
{
    using namespace DAVA;

    InitCrashDumpHandler();

    Assert::AddHandler(Assert::DefaultLoggerHandler);

    Vector<String> modules =
    {
      "JobManager",
      "NetCore"
    };

    KeyedArchive* options = new KeyedArchive; // options will be placed into RefPtr inside of Engine
    options->SetBool("separate_net_thread", true);

    Engine e;
    e.Init(eEngineRunMode::CONSOLE_MODE, modules, options);

    e.update.Connect([&e](float32)
                     {
                         int result = Process(e);
                         e.QuitAsync(result);
                     });

    return e.Run();
}
