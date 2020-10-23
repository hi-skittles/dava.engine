#include "Classes/SlotSupportModule/Private/EntityForSlotLoader.h"

#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Deprecated/SceneValidator.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/Systems/SelectionSystem.h>
#include <REPlatform/Scene/Systems/StructureSystem.h>

#include <TArc/Core/ContextAccessor.h>

#include <Base/TemplateHelpers.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/CustomPropertiesComponent.h>

EntityForSlotLoader::EntityForSlotLoader(DAVA::ContextAccessor* accessor_)
    : accessor(accessor_)
{
}

void EntityForSlotLoader::Load(DAVA::RefPtr<DAVA::Entity> rootEntity, const DAVA::FilePath& path, const DAVA::Function<void(DAVA::String&&)>& finishCallback)
{
    using namespace DAVA;
    DVASSERT(scene != nullptr);
    SceneEditor2* editorScene = DAVA::DynamicTypeCheck<SceneEditor2*>(scene);

    DAVA::Entity* loadedEntity = editorScene->GetSystem<StructureSystem>()->Load(path);
    CallbackInfo callbackInfo;
    callbackInfo.callback = finishCallback;

    if (loadedEntity != nullptr)
    {
        loadedEntity->SetNotRemovable(true);
        DAVA::CustomPropertiesComponent* propertiesComponent = DAVA::GetOrCreateCustomProperties(loadedEntity);
        propertiesComponent->GetArchive()->SetBool(DAVA::ResourceEditor::EDITOR_IS_LOCKED, true);
        rootEntity->AddNode(loadedEntity);
        rootEntity->SetNotRemovable(true);
    }
    else
    {
        callbackInfo.message = DAVA::Format("Can't load item: %s", path.GetStringValue().c_str());
    }

    callbacks.push_back(callbackInfo);
    SafeRelease(loadedEntity);
}

void EntityForSlotLoader::AddEntity(DAVA::Entity* parent, DAVA::Entity* child)
{
    using namespace DAVA;
    DVASSERT(scene != nullptr);
    SceneEditor2* editorScene = DAVA::DynamicTypeCheck<SceneEditor2*>(scene);

    child->SetNotRemovable(true);
    DAVA::CustomPropertiesComponent* propertiesComponent = DAVA::GetOrCreateCustomProperties(child);
    propertiesComponent->GetArchive()->SetBool(ResourceEditor::EDITOR_CONST_REFERENCE, true);

    SelectionSystem* selectionSystem = editorScene->GetSystem<SelectionSystem>();
    DVASSERT(selectionSystem != nullptr);

    bool isLocked = selectionSystem->IsLocked();
    selectionSystem->SetLocked(true);
    parent->AddNode(child);
    selectionSystem->SetLocked(isLocked);

    SceneValidator validator;
    ProjectManagerData* data = accessor->GetGlobalContext()->GetData<ProjectManagerData>();
    if (data)
    {
        validator.SetPathForChecking(data->GetProjectPath());
    }
    validator.ValidateScene(editorScene, editorScene->GetScenePath());
}

void EntityForSlotLoader::Process(DAVA::float32 delta)
{
    for (auto& callbackNode : callbacks)
    {
        callbackNode.callback(std::move(callbackNode.message));
    }

    callbacks.clear();
}

void EntityForSlotLoader::Reset()
{
    callbacks.clear();
}
