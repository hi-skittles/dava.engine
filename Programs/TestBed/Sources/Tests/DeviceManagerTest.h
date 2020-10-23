#pragma once

#include "Infrastructure/BaseScreen.h"

#include <DeviceManager/DeviceManagerTypes.h>

namespace DAVA
{
class DeviceManager;
}

class TestBed;
class DeviceManagerTest : public BaseScreen
{
public:
    DeviceManagerTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    void OnDisplayConfigChanged();
    void OnDisplayClick(DAVA::BaseObject*, void* args, void*);

    DAVA::DeviceManager* deviceManager = nullptr;
    DAVA::Vector<DAVA::UIStaticText*> uiDisplays;
    DAVA::UIStaticText* uiDisplayDescr = nullptr;
    DAVA::Vector<DAVA::DisplayInfo> displays;
};
