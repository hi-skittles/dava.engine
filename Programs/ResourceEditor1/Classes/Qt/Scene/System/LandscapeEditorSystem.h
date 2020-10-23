#pragma once

#include "Classes/Qt/Scene/System/EditorSceneSystem.h"
#include "Classes/Qt/Scene/System/LandscapeEditorDrawSystem.h"

#include "DAVAEngine.h"

class SceneCollisionSystem;
class EntityModificationSystem;

class LandscapeEditorSystem : public DAVA::SceneSystem, public EditorSceneSystem
{
public:
    LandscapeEditorSystem(DAVA::Scene* scene, const DAVA::FilePath& cursorPathname);
    ~LandscapeEditorSystem() override;

    bool IsLandscapeEditingEnabled() const;

protected:
    LandscapeEditorDrawSystem::eErrorType IsCanBeEnabled() const;

    void UpdateCursorPosition();
    void RenderRestoreCallback();

protected:
    SceneCollisionSystem* collisionSystem = nullptr;
    EntityModificationSystem* modifSystem = nullptr;
    LandscapeEditorDrawSystem* drawSystem = nullptr;
    DAVA::FilePath cursorPathName;
    DAVA::Vector2 cursorPosition;
    DAVA::Vector2 prevCursorPos;
    DAVA::Texture* cursorTexture = nullptr;
    DAVA::float32 cursorSize = 0.0f;
    DAVA::float32 landscapeSize = 0.0f;
    bool isIntersectsLandscape = false;
    bool enabled = false;
};
