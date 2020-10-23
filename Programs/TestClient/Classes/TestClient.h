#pragma once

#include <Base/BaseTypes.h>

namespace DAVA
{
class Engine;
class Window;
}

class TestClient
{
public:
    TestClient(DAVA::Engine& engine);

    void OnLoopStarted();
    void OnLoopStopped();
    void OnEngineCleanup();

    void OnSuspended();
    void OnResumed();

    void OnUpdate(DAVA::float32 frameDelta);

private:
    DAVA::Engine& engine;
};
