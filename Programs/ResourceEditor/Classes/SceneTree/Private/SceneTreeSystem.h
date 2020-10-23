#pragma once

#include <REPlatform/DataNodes/Selectable.h>
#include <REPlatform/Scene/Systems/EditorSceneSystem.h>
#include <REPlatform/Commands/RECommandNotificationObject.h>

#include <Base/BaseTypes.h>
#include <Command/Command.h>
#include <Entity/SceneSystem.h>
#include <Functional/Signal.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Scene.h>

class RECommandNotificationObject;

class SceneTreeSystem : public DAVA::SceneSystem, public DAVA::EditorSceneSystem
{
public:
    SceneTreeSystem(DAVA::Scene* scene);

    void RegisterEntity(DAVA::Entity* entity) override;
    void RegisterComponent(DAVA::Entity* entity, DAVA::Component* component) override;

    void UnregisterEntity(DAVA::Entity* entity) override;
    void UnregisterComponent(DAVA::Entity* entity, DAVA::Component* component) override;
    void PrepareForRemove() override;

    void Process(DAVA::float32 timeElapsed) override;
    void ProcessCommand(const DAVA::RECommandNotificationObject& commandNotification) override;

    struct SyncSnapshot
    {
        DAVA::Map<DAVA::uint32, DAVA::Vector<DAVA::Selectable>> objectsToRefetch;
        DAVA::Map<DAVA::uint32, DAVA::Vector<DAVA::Selectable>, std::greater<DAVA::uint32>> removedObjects;

        DAVA::UnorderedSet<DAVA::Selectable> changedObjects;

        bool IsEmpty() const;
    };

    const SyncSnapshot& GetSyncSnapshot() const;
    void SyncFinished();
    DAVA::Signal<> syncIsNecessary;

protected:
    std::unique_ptr<DAVA::Command> PrepareForSave(bool saveForGame) override;

private:
    SyncSnapshot syncSnapshot;
    bool syncRequested = false;
};

inline bool SceneTreeSystem::SyncSnapshot::IsEmpty() const
{
    return objectsToRefetch.empty() == true && removedObjects.empty() == true && changedObjects.empty() == true;
}
