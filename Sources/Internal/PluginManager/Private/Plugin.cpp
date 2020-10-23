
///
#include "Base/Platform.h"
#include "PluginManager/Plugin.h"
#include "Utils/UTF8Utils.h"

#if defined(__DAVAENGINE_MACOS__) 

#include <dlfcn.h>

#endif

namespace DAVA
{
///

#if defined(__DAVAENGINE_MACOS__)

PluginHandle OpenPlugin(const char* pluginPath)
{
    return dlopen(pluginPath, RTLD_NOW);
}

void* LoadFunction(PluginHandle handle, const char* funcName)
{
    return dlsym(handle, funcName);
}

void ClosePlugin(PluginHandle handle)
{
    dlclose(handle);
}

#elif defined(__DAVAENGINE_WIN32__) && !defined(__DAVAENGINE_WIN_UAP__)

PluginHandle OpenPlugin(const char* pluginPath)
{
    WideString pluginWcharPath = DAVA::UTF8Utils::EncodeToWideString(pluginPath);
    return LoadLibraryW(pluginWcharPath.c_str());
}

void* LoadFunction(PluginHandle handle, const char* funcName)
{
    return GetProcAddress(static_cast<HMODULE>(handle), funcName);
}

void ClosePlugin(PluginHandle handle)
{
    FreeLibrary(static_cast<HMODULE>(handle));
}

#else

PluginHandle OpenPlugin(const char* pluginPath)
{
    return nullptr;
}

void* LoadFunction(PluginHandle handle, const char* funcName)
{
    return nullptr;
}

void ClosePlugin(PluginHandle handle)
{
}
 
#endif
}
