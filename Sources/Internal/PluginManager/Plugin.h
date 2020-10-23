#pragma once

#if defined(__DAVAENGINE_MACOS__) 

#define PLUGIN_FUNCTION_EXPORT __attribute__((visibility("default")))

#elif defined(__DAVAENGINE_WIN32__) && !defined(__DAVAENGINE_WIN_UAP__)

#define PLUGIN_FUNCTION_EXPORT __declspec(dllexport)

#else

#define PLUGIN_FUNCTION_EXPORT 

#endif

//

#define EXPORT_PLUGIN(PLUGIN) \
extern "C" { \
    PLUGIN_FUNCTION_EXPORT \
    DAVA::IModule* CreatePlugin(DAVA::Engine* engine)\
    {\
        return new PLUGIN(engine);\
    }\
    PLUGIN_FUNCTION_EXPORT\
    void DestroyPlugin(DAVA::IModule* plugin)\
    {\
        delete plugin;\
    }\
}
namespace DAVA
{
/**
Handle plugin
*/
using PluginHandle = void*;

///

/**
Return the loaded plugin handle on pluginPath path
*/
PluginHandle OpenPlugin(const char* pluginPath);

/**
Return a pointer to function of the the plugin
*/
void* LoadFunction(PluginHandle handle, const char* funcName);

/**
Unloads plugin on handles
*/
void ClosePlugin(PluginHandle handle);

/**
 Return a pointer to function of the the plugin
*/
template <class T>
T LoadFunction(PluginHandle handle, const char* funcName)
{
    return reinterpret_cast<T>(LoadFunction(handle, funcName));
}
}
