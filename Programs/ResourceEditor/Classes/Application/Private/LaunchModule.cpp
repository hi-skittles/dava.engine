#include "Classes/Application/LaunchModule.h"

#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/Selectable.h>
#include <REPlatform/Global/GlobalOperations.h>
#include <REPlatform/Global/StringConstants.h>
#include <REPlatform/Scene/BaseTransformProxies.h>

#include <Version/Version.h>

#include <TArc/DataProcessing/DataListener.h>
#include <TArc/DataProcessing/DataWrapper.h>
#include <TArc/Utils/ModuleCollection.h>
#include <TArc/Core/Deprecated.h>

#include <FileSystem/FileSystem.h>
#include <FileSystem/ResourceArchive.h>
#include <Particles/ParticleEmitterInstance.h>
#include <Particles/ParticleForce.h>

class LaunchModule::FirstSceneCreator : public QObject, private DAVA::DataListener
{
public:
    FirstSceneCreator(LaunchModule* module_)
        : module(module_)
    {
        DAVA::ContextAccessor* accessor = module->GetAccessor();
        wrapper = accessor->CreateWrapper(DAVA::ReflectedTypeDB::Get<DAVA::ProjectManagerData>());
        wrapper.SetListener(this);
    }

    void OnDataChanged(const DAVA::DataWrapper& w, const DAVA::Vector<DAVA::Any>& fields) override
    {
        if (!wrapper.HasData())
        {
            return;
        }

        DAVA::ContextAccessor* accessor = module->GetAccessor();
        DAVA::ProjectManagerData* data = accessor->GetGlobalContext()->GetData<DAVA::ProjectManagerData>();
        if (data->IsOpened())
        {
            module->InvokeOperation(DAVA::CreateFirstSceneOperation.ID);
            wrapper.SetListener(nullptr);
            deleteLater();
        }
    }

private:
    LaunchModule* module = nullptr;
    DAVA::DataWrapper wrapper;
};

LaunchModule::~LaunchModule()
{
    DAVA::Selectable::RemoveAllTransformProxies();
}

void LaunchModule::PostInit()
{
    DAVA::Selectable::AddTransformProxyForClass<DAVA::Entity, DAVA::EntityTransformProxy>();
    DAVA::Selectable::AddTransformProxyForClass<DAVA::ParticleEmitterInstance, DAVA::EmitterTransformProxy>();
    DAVA::Selectable::AddTransformProxyForClass<DAVA::ParticleForce, DAVA::ParticleForceTransformProxy>();

    delayedExecutor.DelayedExecute([this]() {
        InvokeOperation(DAVA::OpenLastProjectOperation.ID);
    });

    UnpackHelpDoc();
    new FirstSceneCreator(this);
}

void LaunchModule::UnpackHelpDoc()
{
    DAVA::PropertiesItem versionsInfo = GetAccessor()->CreatePropertiesNode("VersionsInfo");
    const DAVA::EngineContext* engineContext = GetAccessor()->GetEngineContext();
    DAVA::String editorVer = versionsInfo.Get("EditorVersion", DAVA::String(""));

    DAVA::FilePath docsPath = DAVA::FilePath(DAVA::ResourceEditor::DOCUMENTATION_PATH);
    DAVA::String title = DAVA::Version::CreateAppVersion("Resource Editor");
    if (editorVer != title || !engineContext->fileSystem->Exists(docsPath))
    {
        DAVA::Logger::FrameworkDebug("Unpacking Help...");
        try
        {
            DAVA::ResourceArchive helpRA("~res:/ResourceEditor/Help.docs");
            engineContext->fileSystem->DeleteDirectory(docsPath);
            engineContext->fileSystem->CreateDirectory(docsPath, true);
            helpRA.UnpackToFolder(docsPath);
        }
        catch (std::exception& ex)
        {
            DAVA::Logger::Error("can't unpack Help.docs: %s", ex.what());
            DVASSERT(false && "can't upack Help.docs");
        }
    }
    versionsInfo.Set("EditorVersion", title);
}
