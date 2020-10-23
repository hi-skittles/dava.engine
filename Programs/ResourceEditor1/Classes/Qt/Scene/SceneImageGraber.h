#pragma once

#include "Functional/Function.h"
#include "Base/RefPtr.h"
#include "Math/Math2D.h"
#include "FileSystem/FilePath.h"

class SceneEditor2;
class GlobalOperations;

namespace DAVA
{
class Camera;
class Texture;
class Scene;
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
