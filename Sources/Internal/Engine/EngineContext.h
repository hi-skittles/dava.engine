#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class Logger;
class FileSystem;
class AllocatorFactory;
class Random;
class PerformanceSettings;
class VersionInfo;
class ObjectFactory;

class JobManager;
class LocalizationSystem;
class DownloadManager;

class InputSystem;
class ActionSystem;
class UIControlSystem;
class DynamicAtlasSystem;

class SoundSystem;
class AnimationManager;
class FontManager;
class RenderSystem2D;
class UIScreenManager;
class LocalNotificationController;

class DLCManager;
class AssetsManagerAndroid;
class ModuleManager;
class PluginManager;
class EngineSettings;

class DeviceManager;

class InputBindingListener;

class AutotestingSystem;
class ComponentManager;

class TypeDB;
class FastNameDB;
class ReflectedTypeDB;

class DebugOverlay;
class ImageConverter;

namespace Net
{
class NetCore;
}

namespace Analytics
{
class Core;
}

class EngineContext final
{
public:
    // Subsystems that are created on demand
    JobManager* jobManager = nullptr;
    LocalizationSystem* localizationSystem = nullptr;
    DownloadManager* downloadManager = nullptr;
    Net::NetCore* netCore = nullptr;
    SoundSystem* soundSystem = nullptr;

    // Subsystems that are always created
    Logger* logger = nullptr;
    FileSystem* fileSystem = nullptr;
    AllocatorFactory* allocatorFactory = nullptr;
    Random* random = nullptr;
    PerformanceSettings* performanceSettings = nullptr;
    VersionInfo* versionInfo = nullptr;

    InputSystem* inputSystem = nullptr;
    ActionSystem* actionSystem = nullptr;
    // TODO: move UI control system to Window
    UIControlSystem* uiControlSystem = nullptr;
    DynamicAtlasSystem* dynamicAtlasSystem = nullptr;

    AnimationManager* animationManager = nullptr;
    FontManager* fontManager = nullptr;
    RenderSystem2D* renderSystem2D = nullptr;
    UIScreenManager* uiScreenManager = nullptr;
    LocalNotificationController* localNotificationController = nullptr;

    ModuleManager* moduleManager = nullptr;
    PluginManager* pluginManager = nullptr;

    DLCManager* dlcManager = nullptr;
    Analytics::Core* analyticsCore = nullptr;

    EngineSettings* settings = nullptr;

#if defined(__DAVAENGINE_ANDROID__)
    AssetsManagerAndroid* assetsManager = nullptr;
#endif

    DeviceManager* deviceManager = nullptr;

    InputBindingListener* inputListener = nullptr;

    AutotestingSystem* autotestingSystem = nullptr;
    ComponentManager* componentManager = nullptr;

    TypeDB* typeDB = nullptr;
    FastNameDB* fastNameDB = nullptr;
    ReflectedTypeDB* reflectedTypeDB = nullptr;

    ObjectFactory* objectFactory = nullptr;

    DebugOverlay* debugOverlay = nullptr;
    ImageConverter* imageConverter = nullptr;
};

// TODO: this is temporary solution
// we are going to think on how to add functions like CurlGlobalInit()
// into engine context, to be able to call them in such way:
//   DAVA::Context::CurlGlobalInit()
//   DAVA::Context::GetKeyboard()
//   etc.
namespace Context
{
void CurlGlobalInit();
void CurlGlobalDeinit();
}

} // namespace DAVA
