#pragma once

#include "Functional/Function.h"
#include "Base/RefPtr.h"
#include "Math/Math2D.h"
#include "FileSystem/FilePath.h"

namespace DAVA
{
class Camera;
class Texture;
class Scene;
class SceneEditor2;
}

namespace SceneImageGrabber
{
struct Params
{
    DAVA::RefPtr<DAVA::Scene> scene;
    DAVA::RefPtr<DAVA::Camera> cameraToGrab;
    DAVA::Size2i imageSize;
    DAVA::FilePath outputFile;
    DAVA::Function<void()> readyCallback;
    bool processInDAVAFrame = true;
};

void GrabImage(Params params);
}
