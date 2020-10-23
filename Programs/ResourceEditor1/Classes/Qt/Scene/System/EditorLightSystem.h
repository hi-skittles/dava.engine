#pragma once
#include "DAVAEngine.h"

#include "Classes/Qt/Scene/System/EditorSceneSystem.h"

class EditorLightSystem final : public DAVA::SceneSystem, public EditorSceneSystem
{
    friend class SceneEditor2;
    friend class EditorScene;

public:
    EditorLightSystem(DAVA::Scene* scene);
    ~EditorLightSystem() override;

    void AddEntity(DAVA::Entity* entity) override;
    void RemoveEntity(DAVA::Entity* entity) override;
    void PrepareForRemove() override;

    void SceneDidLoaded() override;

    void Process(DAVA::float32 timeElapsed) override;

    void SetCameraLightEnabled(bool enabled);
    bool GetCameraLightEnabled() const;

private:
    void UpdateCameraLightState();
    void UpdateCameraLightPosition();

    void AddCameraLightOnScene();
    void RemoveCameraLightFromScene();

private:
    DAVA::Entity* cameraLight = nullptr;
    DAVA::uint32 lightEntities = 0;
    bool isEnabled = true;
};

inline bool EditorLightSystem::GetCameraLightEnabled() const
{
    return isEnabled;
}
