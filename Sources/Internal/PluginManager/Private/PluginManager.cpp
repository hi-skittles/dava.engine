#include "Engine/Engine.h"
#include "Logger/Logger.h"
#include "ModuleManager/IModule.h"
#include "PluginManager/Plugin.h"
#include "PluginManager/PluginManager.h"
#include "Utils/StringUtils.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
using CreatePluginFuncPtr = IModule* (*)(Engine*);
using DestroyPluginFuncPtr = void (*)(IModule*);

struct PluginDescriptor
{
    CreatePluginFuncPtr createPluginFunc;
    DestroyPluginFuncPtr destroyPluginFunc;
    IModule* plugin;
    String pluginAbsPath;

    PluginHandle handle;
};

Vector<FilePath> PluginManager::GetPlugins(const FilePath& folder, eFindPluginMode mode) const
{
    return LookupPlugins(folder, mode);
}

Vector<DAVA::FilePath> PluginManager::LookupPlugins(const FilePath& folder, eFindPluginMode mode)
{
#ifdef __DAVAENGINE_DEBUG__
    bool debugMode = true;
#else
    bool debugMode = false;
#endif

#if defined(__DAVAENGINE_MACOS__)
    String dlibExtension = ".dylib";
#elif defined(__DAVAENGINE_WIN32__)
    String dlibExtension = ".dll";
#else
    String dlibExtension = ".so";
#endif

    String debugSuffix = "Debug";

    Vector<FilePath> pluginsList;

    FileSystem* fs = GetEngineContext()->fileSystem;
    Vector<FilePath> cacheDirContent = fs->EnumerateFilesInDirectory(folder, false);

    for (auto& path : cacheDirContent)
    {
        String fileName = path.GetBasename();
        String fileExtension = path.GetExtension();

        bool debugLib = StringUtils::EndsWith(fileName, debugSuffix);

        if (fileExtension == dlibExtension)
        {
            bool isModeMatched = false;

            switch (mode)
            {
            case Auto:
                isModeMatched = debugMode == debugLib;
                break;

            case Release:
                isModeMatched = !debugLib;

                break;

            case Debug:
                isModeMatched = debugLib;
                break;
            }

            if (isModeMatched)
            {
                pluginsList.push_back(path);
            }
        }
    }

    return pluginsList;
}

const PluginDescriptor* PluginManager::LoadPlugin(const FilePath& pluginPath)
{
    PluginDescriptor desc;

    bool success = true;
    String pluginAbsPath = pluginPath.GetAbsolutePathname();

    //Check if plugin library has been loaded
    auto FindPlugin = [pluginAbsPath](PluginDescriptor& d)
    {
        return pluginAbsPath == d.pluginAbsPath;
    };

    auto it = std::find_if(begin(pluginDescriptors), end(pluginDescriptors), FindPlugin);
    if (it != pluginDescriptors.end())
    {
        DVASSERT(false, Format("On this path [ %s ] of the plugin is loaded", pluginAbsPath.c_str()).c_str());
        return nullptr;
    }

    // Open the library.
    desc.handle = OpenPlugin(pluginAbsPath.c_str());
    if (nullptr == desc.handle)
    {
        Logger::Debug("[%s] Unable to open library: %s\n", __FILE__, pluginAbsPath.c_str());
        return nullptr;
    }

    desc.pluginAbsPath = pluginPath.GetAbsolutePathname();
    desc.createPluginFunc = LoadFunction<CreatePluginFuncPtr>(desc.handle, "CreatePlugin");
    desc.destroyPluginFunc = LoadFunction<DestroyPluginFuncPtr>(desc.handle, "DestroyPlugin");

    if (nullptr == desc.createPluginFunc)
    {
        Logger::Debug("[%s] Unable to get symbol: %s\n", __FILE__, "CreatePlugin");
        success = false;
    }

    if (nullptr == desc.destroyPluginFunc)
    {
        Logger::Debug("[%s] Unable to get symbol: %s\n", __FILE__, "DestroyPlugin");
        success = false;
    }

    if (success)
    {
        desc.plugin = desc.createPluginFunc(rootEngine);

        if (nullptr == desc.plugin)
        {
            Logger::Debug("[%s] Can not create plugin: %s\n", __FILE__, pluginAbsPath.c_str());
            success = false;
        }
    }

    if (!success)
    {
        ClosePlugin(desc.handle);
        return nullptr;
    }

    desc.plugin->Init();

    pluginDescriptors.push_back(desc);

    Logger::Debug("Plugin loaded - %s", desc.pluginAbsPath.c_str());

    return &pluginDescriptors.back();
}

bool PluginManager::UnloadPlugin(const PluginDescriptor* desc)
{
    DVASSERT(desc != nullptr);

    auto FindDesc = [desc](PluginDescriptor& d)
    {
        return &d == desc;
    };

    auto it = std::find_if(begin(pluginDescriptors), end(pluginDescriptors), FindDesc);
    if (it != pluginDescriptors.end())
    {
        desc->plugin->Shutdown();
        desc->destroyPluginFunc(desc->plugin);
        ClosePlugin(desc->handle);
        pluginDescriptors.erase(it);
        return true;
    }

    return false;
}

void PluginManager::UnloadPlugins()
{
    for (auto it = rbegin(pluginDescriptors); it != rend(pluginDescriptors); ++it)
    {
        it->plugin->Shutdown();
    }

    for (auto it = rbegin(pluginDescriptors); it != rend(pluginDescriptors); ++it)
    {
        it->destroyPluginFunc(it->plugin);
        ClosePlugin(it->handle);
        Logger::Debug("Plugin unloaded - %s", it->pluginAbsPath.c_str());
    }

    pluginDescriptors.clear();
}

PluginManager::PluginManager(Engine* engine)
    : rootEngine(engine)
{
}

PluginManager::~PluginManager()
{
    DVASSERT(pluginDescriptors.empty());
}
}
