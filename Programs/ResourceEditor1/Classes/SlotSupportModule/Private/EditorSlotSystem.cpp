#include "Classes/SlotSupportModule/Private/EditorSlotSystem.h"
#include "Classes/SlotSupportModule/Private/SlotTemplatesData.h"
#include "Classes/SlotSupportModule/SlotSystemSettings.h"
#include "Classes/Commands2/SlotCommands.h"
#include "Classes/Commands2/Base/RECommandBatch.h"
#include "Classes/Commands2/SetFieldValueCommand.h"
#include "Classes/Commands2/RemoveComponentCommand.h"
#include "Classes/Commands2/AddComponentCommand.h"
#include "Classes/Commands2/EntityRemoveCommand.h"
#include "Classes/Commands2/Base/RECommandNotificationObject.h"
#include "Classes/Commands2/EntityAddCommand.h"
#include "Classes/Selection/Selection.h"
#include "Classes/Qt/Scene/SceneSignals.h"
#include "Classes/Application/REGlobal.h"

#include <TArc/Core/ContextAccessor.h>
#include <TArc/DataProcessing/DataContext.h>

#include <Scene3D/Systems/SlotSystem.h>
#include <Scene3D/Components/SlotComponent.h>
#include <Scene3D/Components/SingleComponents/TransformSingleComponent.h>
#include <Scene3D/SceneFile/SerializationContext.h>
#include <Scene3D/SceneFile/VersionInfo.h>
#include <Scene3D/Scene.h>
#include <Render/RenderHelper.h>
#include <Render/Highlevel/RenderSystem.h>
#include <Base/BaseTypes.h>
#include <Base/Singleton.h>

#include <Utils/Utils.h>
#include <QObject>

const DAVA::FastName EditorSlotSystem::emptyItemName = DAVA::FastName("Empty");

namespace EditorSlotSystemDetail
{
void DetachSlotForRemovingEntity(DAVA::Entity* entity, SceneEditor2* scene, REDependentCommandsHolder& holder)
{
    for (DAVA::int32 i = 0; i < entity->GetChildrenCount(); ++i)
    {
        DetachSlotForRemovingEntity(entity->GetChild(i), scene, holder);
    }

    for (DAVA::uint32 i = 0; i < entity->GetComponentCount<DAVA::SlotComponent>(); ++i)
    {
        DAVA::SlotComponent* component = entity->GetComponent<DAVA::SlotComponent>(i);
        holder.AddPreCommand(std::make_unique<AttachEntityToSlot>(scene, component, nullptr, DAVA::FastName()));
    }
}
} // namespace EditorSlotSystemDetail

EditorSlotSystem::EditorSlotSystem(DAVA::Scene* scene, DAVA::TArc::ContextAccessor* accessor_)
    : SceneSystem(scene)
    , accessor(accessor_)
{
}

void EditorSlotSystem::AddEntity(DAVA::Entity* entity)
{
    DVASSERT(entity->GetComponentCount<DAVA::SlotComponent>() > 0);
    entities.push_back(entity);
    pendingOnInitialize.insert(entity);
}

void EditorSlotSystem::RemoveEntity(DAVA::Entity* entity)
{
    DVASSERT(entity->GetComponentCount<DAVA::SlotComponent>() > 0);
    DAVA::FindAndRemoveExchangingWithLast(entities, entity);
    pendingOnInitialize.erase(entity);
}

void EditorSlotSystem::AddComponent(DAVA::Entity* entity, DAVA::Component* component)
{
    DVASSERT(component->GetType()->Is<DAVA::SlotComponent>());
    pendingOnInitialize.insert(entity);
    if (entity->GetComponentCount<DAVA::SlotComponent>() == 1)
    {
#if defined(__DAVAENGINE_DEBUG__)
        DVASSERT(std::find(entities.begin(), entities.end(), entity) == entities.end());
#endif
        entities.push_back(entity);
    }
}

void EditorSlotSystem::RemoveComponent(DAVA::Entity* entity, DAVA::Component* component)
{
    DVASSERT(component->GetType()->Is<DAVA::SlotComponent>());
    if (entity->GetComponentCount<DAVA::SlotComponent>() == 1)
    {
        DAVA::FindAndRemoveExchangingWithLast(entities, entity);
        pendingOnInitialize.erase(entity);
    }
}

void EditorSlotSystem::PrepareForRemove()
{
    entities.clear();
    pendingOnInitialize.clear();
    DVASSERT(inClonedState.empty());
}

void EditorSlotSystem::Process(DAVA::float32 timeElapsed)
{
    using namespace DAVA;

    for (Entity* entity : pendingOnInitialize)
    {
        Set<FastName> names;
        Set<SlotComponent*> uninitializedSlots;
        for (uint32 i = 0; i < entity->GetComponentCount<DAVA::SlotComponent>(); ++i)
        {
            SlotComponent* slotComponent = entity->GetComponent<DAVA::SlotComponent>(i);
            FastName slotName = slotComponent->GetSlotName();
            if (slotName.IsValid())
            {
                names.insert(slotName);
            }
            else
            {
                uninitializedSlots.insert(slotComponent);
            }
        }

        uint32 slotIndex = 1;
        for (SlotComponent* component : uninitializedSlots)
        {
            FastName newSlotName(Format("Slot_%u", slotIndex++));
            while (names.count(newSlotName) > 0)
            {
                newSlotName = FastName(Format("Slot_%u", slotIndex++));
            }

            component->SetSlotName(newSlotName);
        }
    }

    SceneEditor2* scene = static_cast<SceneEditor2*>(GetScene());
    SlotSystem* slotSystem = scene->slotSystem;
    if (scene->modifSystem->InCloneState() == false &&
        scene->modifSystem->InCloneDoneState() == false)
    {
        for (Entity* entity : pendingOnInitialize)
        {
            uint32 slotCount = entity->GetComponentCount<DAVA::SlotComponent>();
            for (uint32 slotIndex = 0; slotIndex < slotCount; ++slotIndex)
            {
                SlotComponent* component = entity->GetComponent<DAVA::SlotComponent>();
                Entity* loadedEntity = slotSystem->LookUpLoadedEntity(component);
                if (loadedEntity == nullptr)
                {
                    FastName itemName = GetSuitableItemName(component);
                    if (itemName.IsValid() == false)
                    {
                        RefPtr<Entity> newEntity(new Entity());
                        slotSystem->AttachEntityToSlot(component, newEntity.Get(), emptyItemName);
                    }
                    else
                    {
                        slotSystem->AttachItemToSlot(component, itemName);
                    }
                }
                else
                {
                    loadedEntity->SetName(component->GetSlotName());
                }
            }
        }

        pendingOnInitialize.clear();
    }

    for (Entity* entity : scene->transformSingleComponent->localTransformChanged)
    {
        SlotComponent* slot = scene->slotSystem->LookUpSlot(entity);
        if (slot == nullptr)
        {
            continue;
        }

        Matrix4 jointTranfsorm = scene->slotSystem->GetJointTransform(slot);
        bool inverseSuccessed = jointTranfsorm.Inverse();
        DVASSERT(inverseSuccessed);
        Matrix4 attachmentTransform = entity->GetLocalTransform() * jointTranfsorm;
        scene->slotSystem->SetAttachmentTransform(slot, attachmentTransform);
    }

    for (Entity* entity : entities)
    {
        for (uint32 i = 0; i < entity->GetComponentCount<DAVA::SlotComponent>(); ++i)
        {
            SlotComponent* component = entity->GetComponent<DAVA::SlotComponent>(i);
            Entity* loadedEntity = scene->slotSystem->LookUpLoadedEntity(component);
            if (loadedEntity != nullptr)
            {
                for (int32 j = 0; j < loadedEntity->GetChildrenCount(); ++j)
                {
                    Entity* child = loadedEntity->GetChild(j);
                    if (child->GetLocked() == false)
                    {
                        child->SetLocked(true);
                    }
                }
            }
        }
    }
}

void EditorSlotSystem::WillClone(DAVA::Entity* originalEntity)
{
    auto extractSlots = [this](DAVA::Entity* entity)
    {
        DAVA::uint32 slotCount = entity->GetComponentCount<DAVA::SlotComponent>();
        if (slotCount > 0)
        {
            DAVA::Scene* scene = GetScene();
            for (DAVA::uint32 i = 0; i < slotCount; ++i)
            {
                AttachedItemInfo info;
                info.component = entity->GetComponent<DAVA::SlotComponent>(i);
                DVASSERT(info.component->GetEntity() != nullptr);
                info.entity = DAVA::RefPtr<DAVA::Entity>::ConstructWithRetain(scene->slotSystem->LookUpLoadedEntity(info.component));
                info.itemName = info.component->GetLoadedItemName();

                inClonedState[entity].push_back(info);
                DetachEntity(info.component, info.entity.Get());
            }
        }
    };

    extractSlots(originalEntity);
    DAVA::Vector<DAVA::Entity*> children;
    originalEntity->GetChildEntitiesWithComponent(children, DAVA::Type::Instance<DAVA::SlotComponent>());
    for (DAVA::Entity* e : children)
    {
        extractSlots(e);
    }
}

void EditorSlotSystem::DidCloned(DAVA::Entity* originalEntity, DAVA::Entity* newEntity)
{
    auto restoreSlots = [this](DAVA::Entity* entity)
    {
        auto iter = inClonedState.find(entity);
        if (iter == inClonedState.end())
        {
            return;
        }

        const DAVA::Vector<AttachedItemInfo> infos = iter->second;
        for (const AttachedItemInfo& info : infos)
        {
            AttachEntity(info.component, info.entity.Get(), info.itemName);
        }
        inClonedState.erase(iter);
    };

    restoreSlots(originalEntity);
    DAVA::Vector<DAVA::Entity*> children;
    originalEntity->GetChildEntitiesWithComponent(children, DAVA::Type::Instance<DAVA::SlotComponent>());
    for (DAVA::Entity* e : children)
    {
        restoreSlots(e);
    }
}

DAVA::RefPtr<DAVA::KeyedArchive> EditorSlotSystem::SaveSlotsPreset(DAVA::Entity* entity)
{
    DAVA::RefPtr<DAVA::KeyedArchive> result;
    DAVA::Vector<DAVA::RefPtr<DAVA::KeyedArchive>> archives;
    for (DAVA::int32 i = 0; i < entity->GetChildrenCount(); ++i)
    {
        DAVA::Entity* child = entity->GetChild(i);
        DAVA::RefPtr<DAVA::KeyedArchive> subEntityResult = SaveSlotsPreset(entity->GetChild(i));
        if (subEntityResult.Get() != nullptr)
        {
            archives.emplace_back(subEntityResult);
        }
    }

    if (archives.empty() == false)
    {
        result = DAVA::RefPtr<DAVA::KeyedArchive>(new DAVA::KeyedArchive());
        DAVA::uint32 subEntitiesCount = static_cast<DAVA::uint32>(archives.size());
        result->SetUInt32("subEntitiesCount", subEntitiesCount);
        for (DAVA::uint32 subEntityIndex = 0; subEntityIndex < subEntitiesCount; ++subEntityIndex)
        {
            DAVA::RefPtr<DAVA::KeyedArchive> arch = archives[subEntityIndex];
            result->SetArchive(DAVA::Format("subEntity_%u", subEntityIndex), arch.Get());
        }
    }

    DAVA::uint32 slotsCount = entity->GetComponentCount<DAVA::SlotComponent>();
    if (slotsCount != 0)
    {
        DAVA::FilePath scenePath = static_cast<SceneEditor2*>(GetScene())->GetScenePath();

        DAVA::SerializationContext serializeCtx;
        serializeCtx.SetVersion(DAVA::VersionInfo::Instance()->GetCurrentVersion().version);
        serializeCtx.SetScenePath(scenePath.GetDirectory());
        serializeCtx.SetRootNodePath(scenePath);
        serializeCtx.SetScene(GetScene());

        if (result.Get() == nullptr)
        {
            result = DAVA::RefPtr<DAVA::KeyedArchive>(new DAVA::KeyedArchive());
        }

        result->SetUInt32("slotsCount", slotsCount);
        for (DAVA::uint32 slotIndex = 0; slotIndex < slotsCount; ++slotIndex)
        {
            DAVA::SlotComponent* component = entity->GetComponent<DAVA::SlotComponent>(slotIndex);
            DAVA::RefPtr<DAVA::KeyedArchive> arch(new DAVA::KeyedArchive());
            component->Serialize(arch.Get(), &serializeCtx);
            DVASSERT(component->GetSlotName().IsValid());
            arch->SetFastName("slotName", component->GetSlotName());

            DAVA::FastName loadedItem = component->GetLoadedItemName();
            if (loadedItem.IsValid())
            {
                arch->SetFastName("loadedItem", loadedItem);
            }

            result->SetArchive(DAVA::Format("slot_%u", slotIndex), arch.Get());
        }
    }

    if (result.Get() != nullptr)
    {
        DVASSERT(entity->GetName().IsValid() == true);
        result->SetFastName("entityName", entity->GetName());
    }

    return result;
}

void EditorSlotSystem::LoadSlotsPreset(DAVA::Entity* entity, DAVA::RefPtr<DAVA::KeyedArchive> archive)
{
    SceneEditor2* scene = static_cast<SceneEditor2*>(GetScene());
    scene->BeginBatch("Load slots preset");
    DAVA::FastName rootEntityName = archive->GetFastName("entityName");
    if (entity->GetName() != rootEntityName)
    {
        DAVA::Reflection ref = DAVA::Reflection::Create(DAVA::ReflectedObject(entity));
        DVASSERT(ref.IsValid());
        DAVA::Reflection::Field f;
        f.key = DAVA::Entity::EntityNameFieldName;
        f.ref = ref.GetField(f.key);
        scene->Exec(std::make_unique<SetFieldValueCommand>(f, rootEntityName));
    }
    LoadSlotsPresetImpl(entity, archive);
    scene->EndBatch();
}

DAVA::FastName EditorSlotSystem::GenerateUniqueSlotName(DAVA::SlotComponent* component)
{
    DVASSERT(component->GetEntity() != nullptr);
    return GenerateUniqueSlotName(component, component->GetEntity(), DAVA::FastName(), DAVA::FastName(), DAVA::Set<DAVA::FastName>());
}

DAVA::FastName EditorSlotSystem::GenerateUniqueSlotName(DAVA::SlotComponent* component, DAVA::Entity* entity,
                                                        const DAVA::FastName& newTemplateName, const DAVA::FastName& newEntityName,
                                                        const DAVA::Set<DAVA::FastName>& reservedName)
{
    DAVA::FastName entityFName = newEntityName.IsValid() == true ? newEntityName : entity->GetName();
    DAVA::FastName templateFName = newTemplateName.IsValid() == true ? newTemplateName : component->GetTemplateName();

    DAVA::String entityName = entityFName.IsValid() == true ? entityFName.c_str() : "";
    DAVA::String templateName = templateFName.IsValid() == true ? templateFName.c_str() : "";

    DAVA::String mask = entityName + "_" + templateName + "_";
    DAVA::uint32 index = 0;
    DAVA::FastName nameCandidate;
    bool uniqueNameGenerated = false;
    while (uniqueNameGenerated != true)
    {
        uniqueNameGenerated = true;
        nameCandidate = DAVA::FastName(DAVA::Format("%s%u", mask.c_str(), index));
        if (reservedName.count(nameCandidate) > 0)
        {
            uniqueNameGenerated = false;
        }
        else
        {
            for (DAVA::uint32 i = 0; i < entity->GetComponentCount<DAVA::SlotComponent>(); ++i)
            {
                DAVA::SlotComponent* comp = entity->GetComponent<DAVA::SlotComponent>(i);
                if (comp == component)
                {
                    continue;
                }

                if (comp->GetSlotName() == nameCandidate)
                {
                    uniqueNameGenerated = false;
                    break;
                }
            }
        }

        index++;
    }

    DVASSERT(uniqueNameGenerated == true);

    return nameCandidate;
}

void EditorSlotSystem::DetachEntity(DAVA::SlotComponent* component, DAVA::Entity* entity)
{
    DAVA::Entity* slotEntity = component->GetEntity();
    DAVA::Entity* loadedEntity = GetScene()->slotSystem->LookUpLoadedEntity(component);
    DVASSERT(loadedEntity == entity);
    DVASSERT(slotEntity == entity->GetParent());

    slotEntity->RemoveNode(entity);
}

void EditorSlotSystem::AttachEntity(DAVA::SlotComponent* component, DAVA::Entity* entity, DAVA::FastName itemName)
{
    DAVA::SlotSystem* slotSystem = GetScene()->slotSystem;
    Selection::Lock();
    slotSystem->AttachEntityToSlot(component, entity, itemName);
    Selection::Unlock();
}

DAVA::RefPtr<DAVA::Entity> EditorSlotSystem::AttachEntity(DAVA::SlotComponent* component, DAVA::FastName itemName)
{
    Selection::Lock();
    SCOPE_EXIT
    {
        Selection::Unlock();
    };

    DAVA::SlotSystem* slotSystem = GetScene()->slotSystem;
    if (itemName == emptyItemName)
    {
        DAVA::RefPtr<DAVA::Entity> newEntity(new DAVA::Entity());
        slotSystem->AttachEntityToSlot(component, newEntity.Get(), itemName);
        return newEntity;
    }
    else
    {
        return slotSystem->AttachItemToSlot(component, itemName);
    }
}

void EditorSlotSystem::AccumulateDependentCommands(REDependentCommandsHolder& holder)
{
    bool autoGenerateName = accessor->GetGlobalContext()->GetData<SlotSystemSettings>()->autoGenerateSlotNames;
    SceneEditor2* scene = static_cast<SceneEditor2*>(GetScene());
    auto changeSlotVisitor = [&](const RECommand* command)
    {
        const SetFieldValueCommand* cmd = static_cast<const SetFieldValueCommand*>(command);
        const DAVA::Reflection::Field& field = cmd->GetField();
        DAVA::ReflectedObject object = field.ref.GetDirectObject();
        DAVA::FastName fieldName = field.key.Cast<DAVA::FastName>(DAVA::FastName(""));
        const DAVA::ReflectedType* type = object.GetReflectedType();
        if (type == DAVA::ReflectedTypeDB::Get<DAVA::SlotComponent>())
        {
            DAVA::SlotComponent* component = object.GetPtr<DAVA::SlotComponent>();
            if (fieldName == DAVA::SlotComponent::ConfigPathFieldName)
            {
                DAVA::RefPtr<DAVA::Entity> newEntity(new DAVA::Entity());
                holder.AddPreCommand(std::make_unique<AttachEntityToSlot>(scene, component, newEntity.Get(), emptyItemName));
            }
            else if (autoGenerateName == true && fieldName == DAVA::SlotComponent::TemplateFieldName)
            {
                DAVA::Reflection componentRef = DAVA::Reflection::Create(DAVA::ReflectedObject(component));
                DAVA::Reflection::Field f;
                f.key = DAVA::SlotComponent::SlotNameFieldName;
                f.ref = componentRef.GetField(f.key);
                DVASSERT(f.ref.IsValid());
                DAVA::FastName uniqueName = GenerateUniqueSlotName(component, component->GetEntity(), cmd->GetNewValue().Cast<DAVA::FastName>(), DAVA::FastName(), DAVA::Set<DAVA::FastName>());
                holder.AddPostCommand(std::make_unique<SetFieldValueCommand>(f, uniqueName));
            }
        }
        else if (autoGenerateName == true && type == DAVA::ReflectedTypeDB::Get<DAVA::Entity>() && fieldName == DAVA::Entity::EntityNameFieldName)
        {
            DAVA::Entity* entityWithNewName = object.GetPtr<DAVA::Entity>();
            DAVA::Set<DAVA::FastName> reservedNames;
            for (DAVA::uint32 i = 0; i < entityWithNewName->GetComponentCount<DAVA::SlotComponent>(); ++i)
            {
                DAVA::SlotComponent* component = entityWithNewName->GetComponent<DAVA::SlotComponent>(i);
                DAVA::Reflection componentRef = DAVA::Reflection::Create(DAVA::ReflectedObject(component));
                DAVA::Reflection::Field f;
                f.key = DAVA::SlotComponent::SlotNameFieldName;
                f.ref = componentRef.GetField(f.key);
                DVASSERT(f.ref.IsValid());
                DAVA::FastName uniqueName = GenerateUniqueSlotName(component, component->GetEntity(), DAVA::FastName(), cmd->GetNewValue().Cast<DAVA::FastName>(), reservedNames);
                reservedNames.insert(uniqueName);
                holder.AddPostCommand(std::make_unique<SetFieldValueCommand>(f, uniqueName));
            }
        }
    };

    const RECommandNotificationObject& commandInfo = holder.GetMasterCommandInfo();
    commandInfo.ForEach(changeSlotVisitor, CMDID_REFLECTED_FIELD_MODIFY);

    auto removeSlotVisitor = [&](const RECommand* command)
    {
        const RemoveComponentCommand* cmd = static_cast<const RemoveComponentCommand*>(command);
        DAVA::Component* component = const_cast<DAVA::Component*>(cmd->GetComponent());
        if (component->GetType()->Is<DAVA::SlotComponent>())
        {
            DAVA::SlotComponent* slotComponent = static_cast<DAVA::SlotComponent*>(component);
            holder.AddPreCommand(std::make_unique<AttachEntityToSlot>(scene, slotComponent, nullptr, DAVA::FastName()));
        }
    };

    commandInfo.ForEach(removeSlotVisitor, CMDID_COMPONENT_REMOVE);

    auto removeEntityVisitor = [&](const RECommand* command)
    {
        const EntityRemoveCommand* cmd = static_cast<const EntityRemoveCommand*>(command);
        DAVA::Entity* entityToRemove = cmd->GetEntity();
        EditorSlotSystemDetail::DetachSlotForRemovingEntity(entityToRemove, scene, holder);
    };

    commandInfo.ForEach(removeEntityVisitor, CMDID_ENTITY_REMOVE);

    auto loadDefaultItem = [&](DAVA::SlotComponent* component)
    {
        DAVA::FastName itemName = GetSuitableItemName(component);
        if (itemName.IsValid() == false)
        {
            DAVA::RefPtr<DAVA::Entity> newEntity(new DAVA::Entity());
            holder.AddPostCommand(std::unique_ptr<DAVA::Command>(new AttachEntityToSlot(scene, component, newEntity.Get(), emptyItemName)));
        }
        else
        {
            holder.AddPostCommand(std::unique_ptr<DAVA::Command>(new AttachEntityToSlot(scene, component, itemName)));
        }
    };

    auto addSlotVisitor = [&](const RECommand* command)
    {
        const AddComponentCommand* cmd = static_cast<const AddComponentCommand*>(command);
        DAVA::Component* component = cmd->GetComponent();
        if (component->GetType()->Is<DAVA::SlotComponent>())
        {
            SlotSystemSettings* settings = accessor->GetGlobalContext()->GetData<SlotSystemSettings>();
            DAVA::SlotComponent* slotComponent = static_cast<DAVA::SlotComponent*>(component);
            if (slotComponent->GetConfigFilePath().IsEmpty())
            {
                slotComponent->SetConfigFilePath(settings->lastConfigPath);
            }
            loadDefaultItem(static_cast<DAVA::SlotComponent*>(component));

            if (autoGenerateName == true)
            {
                DAVA::Reflection componentRef = DAVA::Reflection::Create(DAVA::ReflectedObject(component));
                DAVA::Reflection::Field f;
                f.key = DAVA::SlotComponent::SlotNameFieldName;
                f.ref = componentRef.GetField(f.key);
                DVASSERT(f.ref.IsValid());
                DAVA::FastName uniqueName = GenerateUniqueSlotName(slotComponent, cmd->GetEntity(), DAVA::FastName(), DAVA::FastName(), DAVA::Set<DAVA::FastName>());
                holder.AddPostCommand(std::make_unique<SetFieldValueCommand>(f, uniqueName));
            }
        }
    };

    commandInfo.ForEach(addSlotVisitor, CMDID_COMPONENT_ADD);

    auto addEntityVisitor = [&](const RECommand* command)
    {
        const EntityAddCommand* cmd = static_cast<const EntityAddCommand*>(command);
        DAVA::Entity* entityForAdd = cmd->GetEntity();
        DAVA::Vector<DAVA::Entity*> slotEntityes;
        entityForAdd->GetChildEntitiesWithComponent(slotEntityes, DAVA::Type::Instance<DAVA::SlotComponent>());
        slotEntityes.push_back(entityForAdd);

        for (DAVA::Entity* e : slotEntityes)
        {
            for (DAVA::uint32 i = 0; i < e->GetComponentCount<DAVA::SlotComponent>(); ++i)
            {
                loadDefaultItem(e->GetComponent<DAVA::SlotComponent>(i));
            }
        }
    };

    commandInfo.ForEach(addEntityVisitor, CMDID_ENTITY_ADD);
}

void EditorSlotSystem::ProcessCommand(const RECommandNotificationObject& commandNotification)
{
    SceneEditor2* scene = static_cast<SceneEditor2*>(GetScene());
    auto visitor = [&](const RECommand* command)
    {
        const SetFieldValueCommand* cmd = static_cast<const SetFieldValueCommand*>(command);
        const DAVA::Reflection::Field& field = cmd->GetField();
        DAVA::ReflectedObject object = field.ref.GetDirectObject();
        DAVA::FastName fieldName = field.key.Cast<DAVA::FastName>(DAVA::FastName(""));
        const DAVA::ReflectedType* type = object.GetReflectedType();
        if (type == DAVA::ReflectedTypeDB::Get<DAVA::SlotComponent>())
        {
            DAVA::SlotComponent* component = object.GetPtr<DAVA::SlotComponent>();
            if (fieldName == DAVA::SlotComponent::SlotNameFieldName)
            {
                DAVA::Entity* entity = scene->slotSystem->LookUpLoadedEntity(component);
                if (entity != nullptr)
                {
                    entity->SetName(component->GetSlotName());
                }
            }
            else if (fieldName == DAVA::SlotComponent::ConfigPathFieldName)
            {
                SlotSystemSettings* settings = accessor->GetGlobalContext()->GetData<SlotSystemSettings>();
                settings->lastConfigPath = component->GetConfigFilePath();
            }
        }

        if (type == DAVA::ReflectedTypeDB::Get<DAVA::Entity>() && fieldName == DAVA::Entity::EntityNameFieldName)
        {
            DAVA::Entity* entity = object.GetPtr<DAVA::Entity>();
            DAVA::SlotComponent* component = scene->slotSystem->LookUpSlot(entity);
            if (component != nullptr)
            {
                component->SetSlotName(entity->GetName());
            }
        }
    };

    commandNotification.ForEach(visitor, CMDID_REFLECTED_FIELD_MODIFY);
}

DAVA::FastName EditorSlotSystem::GetSuitableItemName(DAVA::SlotComponent* component) const
{
    DAVA::FastName itemName;

    DAVA::Vector<DAVA::SlotSystem::ItemsCache::Item> items = GetScene()->slotSystem->GetItems(component->GetConfigFilePath());
    DAVA::FastName templateName = component->GetTemplateName();
    if (templateName.IsValid())
    {
        auto iter = std::find_if(items.begin(), items.end(), [templateName](const DAVA::SlotSystem::ItemsCache::Item& item) {
            return item.type == templateName;
        });

        if (iter != items.end())
        {
            itemName = iter->itemName;
        }
    }

    return itemName;
}

void EditorSlotSystem::Draw()
{
    using namespace DAVA;
    SlotTemplatesData* data = accessor->GetGlobalContext()->GetData<SlotTemplatesData>();
    Scene* scene = GetScene();

    SlotSystemSettings* settings = REGlobal::GetGlobalContext()->GetData<SlotSystemSettings>();
    Color boxColor = settings->slotBoxColor;
    Color boxEdgeColor = settings->slotBoxEdgesColor;
    Color pivotColor = settings->slotPivotColor;

    RenderHelper* rh = scene->GetRenderSystem()->GetDebugDrawer();
    for (Entity* entity : entities)
    {
        for (uint32 i = 0; i < entity->GetComponentCount<DAVA::SlotComponent>(); ++i)
        {
            SlotComponent* component = entity->GetComponent<DAVA::SlotComponent>(i);
            FastName templateName = component->GetTemplateName();
            const SlotTemplatesData::Template* t = data->GetTemplate(templateName);
            if (t != nullptr)
            {
                Vector3 min(-t->pivot.x, -t->pivot.y, -t->pivot.z);
                Vector3 max(t->boundingBoxSize.x - t->pivot.x,
                            t->boundingBoxSize.y - t->pivot.y,
                            t->boundingBoxSize.z - t->pivot.z);
                AABBox3 box(min, max);
                Entity* loadedEntity = scene->slotSystem->LookUpLoadedEntity(component);
                if (loadedEntity != nullptr)
                {
                    Matrix4 transform = loadedEntity->GetWorldTransform();
                    rh->DrawAABoxTransformed(box, transform, boxColor, RenderHelper::DRAW_SOLID_DEPTH);
                    rh->DrawAABoxTransformed(box, transform, boxEdgeColor, RenderHelper::DRAW_WIRE_DEPTH);

                    Vector3 pivot = Vector3(0.0f, 0.0f, 0.0f) * transform;
                    rh->DrawIcosahedron(pivot, settings->pivotPointSize, pivotColor, RenderHelper::DRAW_SOLID_DEPTH);
                }
            }
        }
    }
}

std::unique_ptr<DAVA::Command> EditorSlotSystem::PrepareForSave(bool /*saveForGame*/)
{
    if (entities.empty())
    {
        return nullptr;
    }

    SceneEditor2* sceneEditor = static_cast<SceneEditor2*>(GetScene());
    DAVA::SlotSystem* slotSystem = sceneEditor->slotSystem;

    RECommandBatch* batchCommand = new RECommandBatch("Prepare for save", static_cast<DAVA::uint32>(entities.size()));
    DAVA::Map<DAVA::int32, DAVA::Vector<DAVA::Entity*>, std::greater<DAVA::int32>> depthEntityMap;
    for (DAVA::Entity* entity : entities)
    {
        DAVA::int32 depth = 0;
        DAVA::Entity* parent = entity;
        while (parent != nullptr)
        {
            ++depth;
            parent = parent->GetParent();
        }
        depthEntityMap[depth].push_back(entity);
    }
    for (const auto& entityNode : depthEntityMap)
    {
        for (DAVA::Entity* entity : entityNode.second)
        {
            for (DAVA::uint32 i = 0; i < entity->GetComponentCount<DAVA::SlotComponent>(); ++i)
            {
                DAVA::SlotComponent* component = entity->GetComponent<DAVA::SlotComponent>(i);
                batchCommand->Add(std::make_unique<AttachEntityToSlot>(sceneEditor, component, nullptr, DAVA::FastName()));
            }
        }
    }

    return std::unique_ptr<DAVA::Command>(batchCommand);
}

void EditorSlotSystem::SetScene(DAVA::Scene* scene)
{
    {
        SceneEditor2* currentScene = static_cast<SceneEditor2*>(GetScene());
        if (currentScene != nullptr)
        {
            currentScene->modifSystem->RemoveDelegate(this);
        }
    }

    SceneSystem::SetScene(scene);

    {
        SceneEditor2* currentScene = static_cast<SceneEditor2*>(GetScene());
        if (currentScene != nullptr)
        {
            currentScene->modifSystem->AddDelegate(this);
        }
    }
}

void EditorSlotSystem::LoadSlotsPresetImpl(DAVA::Entity* entity, DAVA::RefPtr<DAVA::KeyedArchive> archive)
{
    SceneEditor2* scene = static_cast<SceneEditor2*>(GetScene());
    DAVA::FilePath scenePath = scene->GetScenePath();

    DAVA::SerializationContext serializeCtx;
    serializeCtx.SetVersion(DAVA::VersionInfo::Instance()->GetCurrentVersion().version);
    serializeCtx.SetScenePath(scenePath.GetDirectory());
    serializeCtx.SetRootNodePath(scenePath);
    serializeCtx.SetScene(scene);

    DAVA::uint32 slotsCount = archive->GetUInt32("slotsCount", 0);
    if (slotsCount > 0)
    {
        DAVA::UnorderedMap<DAVA::FastName, DAVA::Deque<DAVA::SlotComponent*>> existsComponents;
        for (DAVA::uint32 i = 0; i < entity->GetComponentCount<DAVA::SlotComponent>(); ++i)
        {
            DAVA::SlotComponent* component = entity->GetComponent<DAVA::SlotComponent>(i);
            existsComponents[component->GetSlotName()].push_back(component);
        }

        for (DAVA::uint32 slotArhcIndex = 0; slotArhcIndex < slotsCount; ++slotArhcIndex)
        {
            DAVA::KeyedArchive* slotArch = archive->GetArchive(DAVA::Format("slot_%u", slotArhcIndex));
            DVASSERT(slotArch != nullptr);
            DAVA::FastName slotName = slotArch->GetFastName("slotName");
            DVASSERT(slotName.IsValid() == true);

            auto iter = existsComponents.find(slotName);
            if (iter != existsComponents.end())
            {
                DAVA::Deque<DAVA::SlotComponent*>& components = iter->second;
                if (components.empty() == false)
                {
                    DAVA::SlotComponent* component = components.front();
                    scene->Exec(std::make_unique<AttachEntityToSlot>(scene, component, nullptr, DAVA::FastName()));
                    scene->Exec(std::make_unique<RemoveComponentCommand>(entity, component));
                    components.pop_front();
                }
            }

            DAVA::SlotComponent* newComponent = new DAVA::SlotComponent();
            newComponent->Deserialize(slotArch, &serializeCtx);
            scene->Exec(std::make_unique<AddComponentCommand>(entity, newComponent));
            DAVA::FastName loadedItem = slotArch->GetFastName("loadedItem");
            if (loadedItem.IsValid() == true)
            {
                scene->Exec(std::make_unique<AttachEntityToSlot>(scene, newComponent, loadedItem));
            }
        }
    }

    DAVA::uint32 subEntitiesCount = archive->GetUInt32("subEntitiesCount", 0);
    if (subEntitiesCount > 0)
    {
        DAVA::UnorderedMap<DAVA::FastName, DAVA::Deque<DAVA::Entity*>> childEntities;
        for (DAVA::int32 childIndex = 0; childIndex < entity->GetChildrenCount(); ++childIndex)
        {
            DAVA::Entity* child = entity->GetChild(childIndex);
            DVASSERT(child->GetName().IsValid());
            childEntities[child->GetName()].push_back(child);
        }

        for (DAVA::uint32 subEntityIndex = 0; subEntityIndex < subEntitiesCount; ++subEntityIndex)
        {
            DAVA::KeyedArchive* subEntityArch = archive->GetArchive(DAVA::Format("subEntity_%u", subEntityIndex));
            DVASSERT(subEntityArch != nullptr);
            DAVA::FastName entityName = subEntityArch->GetFastName("entityName");
            auto iter = childEntities.find(entityName);
            if (iter != childEntities.end())
            {
                DAVA::Entity* e = iter->second.front();
                iter->second.pop_front();
                LoadSlotsPresetImpl(e, DAVA::RefPtr<DAVA::KeyedArchive>::ConstructWithRetain(subEntityArch));
            }
            else
            {
                DAVA::Logger::Warning("Entity with name \"%s\" in \"%s\" not found. Preset's subtree will be ignored", entityName.c_str(), entity->GetName().c_str());
            }
        }
    }
}
