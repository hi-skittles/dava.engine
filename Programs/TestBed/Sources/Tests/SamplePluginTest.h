#pragma once

#include "Infrastructure/BaseScreen.h"
#include "PluginManager/PluginManager.h"

class TestBed;

class SamplePluginTest final : public BaseScreen
{
public:
    SamplePluginTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

private:
    DAVA::Engine& engine;
    const DAVA::PluginDescriptor* pluginDescriptor;
};
