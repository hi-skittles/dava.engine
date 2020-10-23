#include "Classes/Application/LaunchModule.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Qt/Scene/BaseTransformProxies.h"
#include "Classes/Qt/Scene/BaseTransformProxies.h"
#include "Classes/Selection/Selectable.h"

#include "Classes/Project/ProjectManagerData.h"
#include "Classes/StringConstants.h"

#include <Version/Version.h>

#include <TArc/DataProcessing/DataListener.h>
#include <TArc/DataProcessing/DataWrapper.h>
#include <TArc/Utils/ModuleCollection.h>

#include <Particles/ParticleEmitterInstance.h>
#include <FileSystem/ResourceArchive.h>
#include <FileSystem/FileSystem.h>

class LaunchModule::FirstSceneCreator : public QObject, private DAVA::TArc::DataListener
{
public:
    FirstSceneCreator(LaunchModule* module_)
        : module(module_)
    {
        DAVA::TArc::ContextAccessor* accessor = module->GetAccessor();
        wrapper = accessor->CreateWrapper(DAVA::ReflectedTypeDB::Get<ProjectManagerData>());
        wrapper.SetListener(this);
    }

    void OnDataChanged(const DAVA::TArc::DataWrapper& w, const DAVA::Vector<DAVA::Any>& fields) override
    {
        if (!wrapper.HasData())
        {
            return;
        }

        DAVA::TArc::ContextAccessor* accessor = module->GetAccessor();
        ProjectManagerData* data = accessor->GetGlobalContext()->GetData<ProjectManagerData>();
        if (data->IsOpened())
        {
            module->InvokeOperation(REGlobal::CreateFirstSceneOperation.ID);
            wrapper.SetListener(nullptr);
            deleteLater();
        }
    }

private:
    LaunchModule* module = nullptr;
    DAVA::TArc::DataWrapper wrapper;
};

LaunchModule::~LaunchModule()
{
    Selectable::RemoveAllTransformProxies();
}

void LaunchModule::PostInit()
{
    Selectable::AddTransformProxyForClass<DAVA::Entity, EntityTransformProxy>();
    Selectable::AddTransformProxyForClass<DAVA::ParticleEmitterInstance, EmitterTransformProxy>();
    Selectable::AddTransformProxyForClass<DAVA::ParticleForce, ParticleForceTransformProxy>();

    delayedExecutor.DelayedExecute([this]() {
        InvokeOperation(REGlobal::OpenLastProjectOperation.ID);
    });

    UnpackHelpDoc();
    new FirstSceneCreator(this);
}

void LaunchModule::UnpackHelpDoc()
{
    DAVA::TArc::PropertiesItem versionsInfo = GetAccessor()->CreatePropertiesNode("VersionsInfo");
    const DAVA::EngineContext* engineContext = GetAccessor()->GetEngineContext();
    DAVA::String editorVer = versionsInfo.Get("EditorVersion", DAVA::String(""));
    DAVA::FilePath docsPath = DAVA::FilePath(ResourceEditor::DOCUMENTATION_PATH);
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
