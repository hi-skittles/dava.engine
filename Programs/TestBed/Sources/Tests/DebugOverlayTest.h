#pragma once

#include "Infrastructure/BaseScreen.h"
#include "Debug/DebugOverlayItem.h"

class TestBed;

namespace DAVA
{
class Window;
}

class TestOverlayItem final : public DAVA::DebugOverlayItem
{
    virtual DAVA::String GetName() const override;
    virtual void Draw() override;
};

class DebugOverlayTest final : public BaseScreen
{
public:
    DebugOverlayTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    void OnWindowUpdate(DAVA::Window* window, DAVA::float32 dt);

    void OnAddItem(DAVA::BaseObject* sender, void* data, void* callerData);
    void OnRemoveItem(DAVA::BaseObject* sender, void* data, void* callerData);
    void OnShowHideOverlay(DAVA::BaseObject* sender, void* data, void* callerData);

    TestOverlayItem testItem;
    bool itemRegistered = false;
};
