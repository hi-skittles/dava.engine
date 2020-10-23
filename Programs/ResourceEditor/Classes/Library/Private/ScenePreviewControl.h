#pragma once

#include "DAVAEngine.h"
#include "Scene3D/Systems/Controller/RotationControllerSystem.h"

class ScenePreviewControl : public DAVA::UI3DView
{
public:
    enum eError
    {
        ERROR_WRONG_EXTENSION = 100,
        ERROR_CANNOT_OPEN_FILE = 101
    };

public:
    ScenePreviewControl(const DAVA::Rect& rect);
    ~ScenePreviewControl() override;

    void Update(DAVA::float32 timeElapsed) override;

    DAVA::int32 OpenScene(const DAVA::FilePath& pathToFile);
    void ReleaseScene();
    void RecreateScene();

private:
    void CreateCamera();
    void SetupCamera();

private:
    //scene controls
    DAVA::Scene* editorScene = nullptr;
    DAVA::RotationControllerSystem* rotationSystem = nullptr;
    DAVA::FilePath currentScenePath;
    bool needSetCamera = false;
};
