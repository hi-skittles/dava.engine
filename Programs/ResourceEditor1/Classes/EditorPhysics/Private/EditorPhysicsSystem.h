#pragma once

#include <Entity/SceneSystem.h>
#include <Math/Matrix4.h>

class EditorPhysicsSystem : public DAVA::SceneSystem
{
public:
    enum class eSimulationState
    {
        STOPPED,
        PLAYING,
        PAUSED
    };

    EditorPhysicsSystem(DAVA::Scene* scene);
    ~EditorPhysicsSystem() = default;

    void RegisterEntity(DAVA::Entity* entity) override;
    void UnregisterEntity(DAVA::Entity* entity) override;

    void RegisterComponent(DAVA::Entity* entity, DAVA::Component* component) override;
    void UnregisterComponent(DAVA::Entity* entity, DAVA::Component* component) override;

    void PrepareForRemove() override;

    void Process(DAVA::float32 timeElapsed) override;

    void SetSimulationState(eSimulationState newState);
    eSimulationState GetSimulationState() const;

private:
    void StoreActualTransform();
    void RestoreTransform();

private:
    struct EntityInfo
    {
        bool restoreLocalTransform;
        DAVA::Matrix4 originalTransform;
        bool isLocked;
    };
    DAVA::UnorderedMap<DAVA::Entity*, EntityInfo> transformMap;
    eSimulationState state = eSimulationState::STOPPED;
};
