#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/FileSystem.h"

namespace DAVA
{
/**
Plugin should implement and export two functions:
1. -   DAVA::IModule* CreatePlugin(DAVA::Engine* engine)
which does or used for create a plugin object
2. -   void DestroyPlugin(DAVA::IModule* plugin)
which does or used for destruction plugin object

For convenience, you can use EXPORT_PLUGIN macro(module name)
that automatically generates code create and load a module.
*/

class IModule;
class Engine;

struct PluginDescriptor;

/**
plugin manager system
*/
class PluginManager final
{
public:
    enum eFindPluginMode
    {
        Auto, //!< auto mode
        Release, //!< only release plugins
        Debug //!< only plugins
    };

    /**
     Returns a list of plugins in the specified mode.
    */
    Vector<FilePath> GetPlugins(const FilePath& folder, eFindPluginMode mode) const;
    static Vector<FilePath> LookupPlugins(const FilePath& folder, eFindPluginMode mode);

    /**
     Load plugin located on the path pluginPath and returns descriptor to it
    */
    const PluginDescriptor* LoadPlugin(const FilePath& pluginPath);

    /**
    Unload plugin on a descriptor and returns the operation result
    */
    bool UnloadPlugin(const PluginDescriptor* desc);

    /**
     Unload all loaded plugins
    */
    void UnloadPlugins();

    PluginManager(Engine* engine);
    ~PluginManager();

private:
    List<PluginDescriptor> pluginDescriptors;
    Engine* rootEngine;
};
}
