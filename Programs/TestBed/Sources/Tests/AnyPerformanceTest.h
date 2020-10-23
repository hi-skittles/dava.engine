#pragma once

#include "Infrastructure/BaseScreen.h"

namespace DAVA
{
class UITextField;
class UIStaticText;
}

class TestBed;
class AnyPerformanceTest : public BaseScreen
{
public:
    AnyPerformanceTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

    DAVA::UITextField* testCount;
    DAVA::UIStaticText* resultCreate;
    DAVA::UIStaticText* resultGetSet;

    DAVA::uint64 GetLoopCount();
    void SetResult(DAVA::UIStaticText*, DAVA::uint64 ms);
    void OnCreateTest(DAVA::BaseObject* sender, void* data, void* callerData);
    void OnGetSetTest(DAVA::BaseObject* sender, void* data, void* callerData);
};
