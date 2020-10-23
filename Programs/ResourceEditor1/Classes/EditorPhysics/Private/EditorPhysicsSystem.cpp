#include "EditorPhysics/Private/EditorPhysicsSystem.h"

#include <Physics/PhysicsModule.h>
#include <Physics/PhysicsSystem.h>
#include <Physics/DynamicBodyComponent.h>
#include <Physics/StaticBodyComponent.h>
#include <Physics/SphereShapeComponent.h>

#include <Engine/Engine.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Scene.h>
#include <Entity/Component.h>

#include <physx/PxRigidDynamic.h>

EditorPhysicsSystem::EditorPhysicsSystem(DAVA::Scene* scene)
    : SceneSystem(scene)
{
    scene->physicsSystem->SetDebugDrawEnabled(true);
    scene->physicsSystem->SetSimulationEnabled(false);
}

void EditorPhysicsSystem::RegisterEntity(DAVA::Entity* entity)
{
    using namespace DAVA;

    if (entity->GetComponentCount<DynamicBodyComponent>() > 0 ||
        entity->GetComponentCount<StaticBodyComponent>() > 0)
    {
        EntityInfo& info = transformMap[entity];
        info.originalTransform = entity->GetWorldTransform();
        info.isLocked = entity->GetLocked();
        info.restoreLocalTransform = false;
    }
    else
    {
        const PhysicsModule* module = GetEngineContext()->moduleManager->GetModule<PhysicsModule>();
        const Vector<const Type*>& shapeComponents = module->GetShapeComponentTypes();

        Vector<CollisionShapeComponent*> shapes;
        for (const Type* shapeType : shapeComponents)
        {
            const size_t shapesCount = entity->GetComponentCount(shapeType);
            if (shapesCount > 0)
            {
                EntityInfo& info = transformMap[entity];
                info.originalTransform = entity->GetLocalTransform();
                info.isLocked = entity->GetLocked();
                info.restoreLocalTransform = true;

                break;
            }
        }
    }
}

void EditorPhysicsSystem::UnregisterEntity(DAVA::Entity* entity)
{
    auto iter = transformMap.find(entity);
    if (iter != transformMap.end())
    {
        transformMap.erase(iter);
    }
}

void EditorPhysicsSystem::RegisterComponent(DAVA::Entity* entity, DAVA::Component* component)
{
    const DAVA::Type* componentType = component->GetType();
    if (componentType->Is<DAVA::StaticBodyComponent>() || componentType->Is<DAVA::DynamicBodyComponent>())
    {
        RegisterEntity(entity);
    }
}

void EditorPhysicsSystem::UnregisterComponent(DAVA::Entity* entity, DAVA::Component* component)
{
    const DAVA::Type* componentType = component->GetType();
    if (componentType->Is<DAVA::StaticBodyComponent>() || componentType->Is<DAVA::DynamicBodyComponent>())
    {
        DAVA::uint32 actorCount = entity->GetComponentCount<DAVA::StaticBodyComponent>();
        actorCount += entity->GetComponentCount<DAVA::DynamicBodyComponent>();
        if (actorCount == 1)
        {
            UnregisterEntity(entity);
        }
    }
}

void EditorPhysicsSystem::PrepareForRemove()
{
    transformMap.clear();
}

void EditorPhysicsSystem::Process(DAVA::float32 timeElapsed)
{
    using namespace DAVA;

    if (state != eSimulationState::STOPPED)
    {
        for (auto& node : transformMap)
        {
            if (node.first->GetLocked() == false)
            {
                node.first->SetLocked(true);
            }
        }
    }
}

void EditorPhysicsSystem::SetSimulationState(eSimulationState newState)
{
    if (newState == state)
    {
        return;
    }

    DAVA::PhysicsSystem* physicsSystem = GetScene()->physicsSystem;

    switch (newState)
    {
    case eSimulationState::PLAYING:
        DVASSERT(physicsSystem->IsSimulationEnabled() == false);
        if (state == eSimulationState::STOPPED)
        {
            StoreActualTransform();
        }
        physicsSystem->SetSimulationEnabled(true);
        break;
    case eSimulationState::PAUSED:
        DVASSERT(physicsSystem->IsSimulationEnabled() == true);
        DVASSERT(state == eSimulationState::PLAYING);
        physicsSystem->SetSimulationEnabled(false);
        break;
    case eSimulationState::STOPPED:
        DVASSERT(physicsSystem->IsSimulationEnabled() == true || state == eSimulationState::PAUSED);
        physicsSystem->SetSimulationEnabled(false);
        RestoreTransform();
        break;
    default:
        break;
    }

    state = newState;
}

EditorPhysicsSystem::eSimulationState EditorPhysicsSystem::GetSimulationState() const
{
    return state;
}

void EditorPhysicsSystem::StoreActualTransform()
{
    for (auto& node : transformMap)
    {
        if (node.second.restoreLocalTransform)
        {
            node.second.originalTransform = node.first->GetLocalTransform();
        }
        else
        {
            node.second.originalTransform = node.first->GetWorldTransform();
        }

        node.second.isLocked = node.first->GetLocked();
    }
}

void EditorPhysicsSystem::RestoreTransform()
{
    for (auto& node : transformMap)
    {
        if (node.second.restoreLocalTransform)
        {
            node.first->SetLocalTransform(node.second.originalTransform);
        }
        else
        {
            node.first->SetWorldTransform(node.second.originalTransform);
        }

        node.first->SetLocked(node.second.isLocked);

        DAVA::PhysicsComponent* component = static_cast<DAVA::PhysicsComponent*>(node.first->GetComponent<DAVA::DynamicBodyComponent>());
        if (component != nullptr)
        {
            physx::PxRigidDynamic* actor = component->GetPxActor()->is<physx::PxRigidDynamic>();
            if (actor != nullptr && actor->getActorFlags().isSet(physx::PxActorFlag::eDISABLE_SIMULATION) == false)
            {
                actor->setLinearVelocity(physx::PxVec3(0.0f, 0.0f, 0.0f));
                actor->setAngularVelocity(physx::PxVec3(0.0f, 0.0f, 0.0f));
            }
        }
    }
}
