#include "Classes/SignatureModule/SignatureModule.h"
#include "Classes/SignatureModule/Private/OwnersSignatureSystem.h"

#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/Scene/SceneEditor2.h>

#include <TArc/Utils/ModuleCollection.h>

#include <Reflection/ReflectionRegistrator.h>

namespace SignatureModuleDetail
{
DAVA::String GetUserName()
{
    QString name = qgetenv("USER"); //OSX
    if (name.isEmpty())
        name = qgetenv("USERNAME"); //WIN

    return name.toStdString();
}
}

void SignatureModule::PostInit()
{
    currentUserName = SignatureModuleDetail::GetUserName();
}

void SignatureModule::OnContextCreated(DAVA::DataContext* context)
{
    using namespace DAVA;
    SceneData* sceneData = context->GetData<SceneData>();
    SceneEditor2* scene = sceneData->GetScene().Get();
    DVASSERT(scene != nullptr);

    OwnersSignatureSystem* ownerSystem = new OwnersSignatureSystem(scene, currentUserName);
    scene->AddSystem(ownerSystem, 0, DAVA::Scene::SCENE_SYSTEM_REQUIRE_PROCESS);
    ownerSystem->EnableSystem();
}

DAVA_VIRTUAL_REFLECTION_IMPL(SignatureModule)
{
    DAVA::ReflectionRegistrator<SignatureModule>::Begin()
    .ConstructorByPointer()
    .End();
}

DECL_TARC_MODULE(SignatureModule);
