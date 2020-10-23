#pragma once

#include "DAVAEngine.h"
#include "Infrastructure/BaseScreen.h"

class TestBed;
class OnLowMemorySignalTest : public BaseScreen
{
public:
    OnLowMemorySignalTest(TestBed& app);

    void OnLowMemory();

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    void OnUpdate(DAVA::float32);
    void ToggleTest();
    void CleanUp();
    int GetChunkSize();

    const int maxChunkSize = 128;
    const int minChunkSize = 1;

    bool isTestRunning = false;
    DAVA::uint64 numberOfAllocatedMbytes = 0;
    size_t numberOfCallbackCalls = 0;
    DAVA::Vector<DAVA::Vector<DAVA::uint8>> memoryChunks;
};
