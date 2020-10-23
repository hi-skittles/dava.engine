#include "Tests/SamplePluginTest.h"
#include "Infrastructure/TestBed.h"
#include "Engine/Engine.h"

using namespace DAVA;

SamplePluginTest::SamplePluginTest(TestBed& app)
    : BaseScreen(app, "Sample_plugin_test")
    , engine(app.GetEngine())
    , pluginDescriptor(nullptr)
{
}

void SamplePluginTest::LoadResources()
{
    BaseScreen::LoadResources();

    PluginManager& mm = *engine.GetContext()->pluginManager;
    FileSystem& ff = *engine.GetContext()->fileSystem;

    FilePath pluginDir = ff.GetPluginDirectory();

    Vector<FilePath> pluginsList;
    pluginsList = mm.GetPlugins(pluginDir, PluginManager::Auto);

    for (auto& path : pluginsList)
    {
        pluginDescriptor = mm.LoadPlugin(path);
        DVASSERT(pluginDescriptor != nullptr);
    }
}

void SamplePluginTest::UnloadResources()
{
    BaseScreen::UnloadResources();

    PluginManager& mm = *engine.GetContext()->pluginManager;
    mm.UnloadPlugin(pluginDescriptor);
}
