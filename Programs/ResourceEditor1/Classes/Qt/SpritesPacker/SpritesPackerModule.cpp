#include "SpritesPacker/SpritesPackerModule.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/Project/ProjectManagerData.h"
#include "Classes/Application/RESettings.h"

#include <TArc/DataProcessing/DataContext.h>
#include <TArc/WindowSubSystem/UI.h>

#include <QtTools/ReloadSprites/DialogReloadSprites.h>
#include <QtTools/ReloadSprites/SpritesPacker.h>

#include <AssetCache/AssetCacheClient.h>

#include <Engine/Engine.h>
#include <Job/JobManager.h>
#include <Render/2D/Sprite.h>
#include <Functional/Function.h>

#include <QAction>
#include <QDir>

SpritesPackerModule::SpritesPackerModule(DAVA::TArc::UI* ui_, DAVA::TArc::ContextAccessor* accessor_)
    : QObject(nullptr)
    , spritesPacker(new SpritesPacker())
    , ui(ui_)
    , accessor(accessor_)
{
    qRegisterMetaType<DAVA::eGPUFamily>("DAVA::eGPUFamily");
    qRegisterMetaType<DAVA::TextureConverter::eConvertQuality>("DAVA::TextureConverter::eConvertQuality");
}

SpritesPackerModule::~SpritesPackerModule()
{
    spritesPacker->Cancel();
    spritesPacker->ClearTasks();

    if (cacheClient != nullptr)
    {
        DisconnectCacheClient();
    }

    DAVA::GetEngineContext()->jobManager->WaitWorkerJobs();
}

void SpritesPackerModule::RepackWithDialog()
{
    ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
    DVASSERT(data != nullptr);
    SetupSpritesPacker(data->GetProjectPath());

    DAVA::GetEngineContext()->jobManager->CreateWorkerJob(DAVA::MakeFunction(this, &SpritesPackerModule::ConnectCacheClient));

    ShowPackerDialog();

    DisconnectCacheClient();

    ReloadObjects();
}

void SpritesPackerModule::RepackImmediately(const DAVA::FilePath& projectPath, DAVA::eGPUFamily gpu)
{
    SetupSpritesPacker(projectPath);
    CreateWaitDialog(projectPath);

    DAVA::Function<void()> fn = DAVA::Bind(&SpritesPackerModule::ProcessSilentPacking, this, true, false, gpu, DAVA::TextureConverter::ECQ_DEFAULT);
    DAVA::GetEngineContext()->jobManager->CreateWorkerJob(fn);
}

void SpritesPackerModule::SetupSpritesPacker(const DAVA::FilePath& projectPath)
{
    DAVA::FilePath inputDir = projectPath + "/DataSource/Gfx/Particles";
    DAVA::FilePath outputDir = projectPath + "/Data/Gfx/Particles";

    spritesPacker->ClearTasks();
    spritesPacker->AddTask(QString::fromStdString(inputDir.GetAbsolutePathname()), QString::fromStdString(outputDir.GetAbsolutePathname()));
}

void SpritesPackerModule::ProcessSilentPacking(bool clearDirs, bool forceRepack, const DAVA::eGPUFamily gpu, const DAVA::TextureConverter::eConvertQuality quality)
{
    ConnectCacheClient();
    spritesPacker->ReloadSprites(clearDirs, forceRepack, gpu, quality);
    DisconnectCacheClient();

    DAVA::GetEngineContext()->jobManager->CreateMainJob(DAVA::MakeFunction(this, &SpritesPackerModule::CloseWaitDialog));
    DAVA::GetEngineContext()->jobManager->CreateMainJob(DAVA::MakeFunction(this, &SpritesPackerModule::ReloadObjects));
}

void SpritesPackerModule::ShowPackerDialog()
{
    DialogReloadSprites dialogReloadSprites(accessor, spritesPacker.get(), nullptr);
    ui->ShowModalDialog(DAVA::TArc::mainWindowKey, &dialogReloadSprites);
}

void SpritesPackerModule::CreateWaitDialog(const DAVA::FilePath& projectPath)
{
    DAVA::TArc::WaitDialogParams params;
    params.message = QString::fromStdString(DAVA::String("Reloading Particles for ") + projectPath.GetAbsolutePathname());
    params.needProgressBar = false;
    waitDialogHandle = ui->ShowWaitDialog(DAVA::TArc::mainWindowKey, params);
}

void SpritesPackerModule::CloseWaitDialog()
{
    waitDialogHandle.reset();
}

void SpritesPackerModule::ReloadObjects()
{
    DAVA::Sprite::ReloadSprites();

    const DAVA::Vector<DAVA::eGPUFamily>& gpus = spritesPacker->GetResourcePacker().requestedGPUs;
    if (gpus.empty() == false)
    {
        REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->spritesViewGPU = gpus[0];
    }

    emit SpritesReloaded();
}

void SpritesPackerModule::ConnectCacheClient()
{
    DVASSERT(cacheClient == nullptr);
    GeneralSettings* settings = REGlobal::GetGlobalContext()->GetData<GeneralSettings>();
    if (settings->useAssetCache == true)
    {
        DAVA::String ipStr = settings->assetCacheIP;
        DAVA::uint16 port = settings->assetCachePort;
        DAVA::uint64 timeoutSec = settings->assetCacheTimeout;

        DAVA::AssetCacheClient::ConnectionParams params;
        params.ip = (ipStr.empty() ? DAVA::AssetCache::GetLocalHost() : ipStr);
        params.port = port;
        params.timeoutms = timeoutSec * 1000; //in ms

        cacheClient = new DAVA::AssetCacheClient();
        DAVA::AssetCache::Error connected = cacheClient->ConnectSynchronously(params);
        if (connected != DAVA::AssetCache::Error::NO_ERRORS)
        {
            DisconnectCacheClient();
        }
    }

    SetCacheClientForPacker();
}

void SpritesPackerModule::DisconnectCacheClient()
{
    if (cacheClient != nullptr)
    {
        DAVA::AssetCacheClient* disconnectingClient = cacheClient;
        cacheClient = nullptr;
        SetCacheClientForPacker();

        //we should destroy cache client on main thread
        DAVA::GetEngineContext()->jobManager->CreateMainJob(DAVA::Bind(&SpritesPackerModule::DisconnectCacheClientInternal, this, disconnectingClient));
    }
}

void SpritesPackerModule::SetCacheClientForPacker()
{
    spritesPacker->SetCacheClient(cacheClient, "ResourceEditor.ReloadParticles");
}

void SpritesPackerModule::DisconnectCacheClientInternal(DAVA::AssetCacheClient* cacheClientForDisconnect)
{
    DVASSERT(cacheClientForDisconnect != nullptr);

    cacheClientForDisconnect->Disconnect();
    SafeDelete(cacheClientForDisconnect);
}

bool SpritesPackerModule::IsRunning() const
{
    return spritesPacker->IsRunning();
}
