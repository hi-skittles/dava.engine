#pragma once

#include "Base/BaseTypes.h"
#include "DeviceManager/DeviceManagerTypes.h"
#include "Input/TouchScreen.h"
#include "Engine/Private/EnginePrivateFwd.h"
#include "Functional/Signal.h"

/**
    \defgroup device_manager Device Manager
*/

namespace DAVA
{
class Gamepad;
class InputDevice;
class Keyboard;
class Mouse;
namespace Private
{
struct DeviceManagerImpl;
struct MainDispatcherEvent;
}

// TODO: add notifying when input device is added/removed

/**
    \ingroup device_manager

    Class which keeps current device configuration, listens for device addition, removal or devices' properties changes.
    Application can subscribe to appropriate signals to receive notification about configuration changes.

    \todo For now `DeviceManager` observes only display devices, input and cpu stats, further add other devices (storage, maybe network).
*/
class DeviceManager final
{
private:
    DeviceManager(Private::EngineBackend* engineBackend);
    ~DeviceManager();

public:
    // Display methods

    /** Get primary display as reported by system */
    const DisplayInfo& GetPrimaryDisplay() const;

    /** Get displays which are available now */
    const Vector<DisplayInfo>& GetDisplays() const;

    /** Get total display count */
    size_t GetDisplayCount() const;

    // Input methods

    InputDevice* GetInputDevice(uint32 id);
    Gamepad* GetGamepad();
    Keyboard* GetKeyboard();
    Mouse* GetMouse();
    TouchScreen* GetTouchScreen();

    // Signals

    Signal<> displayConfigChanged; //<! Emited when display has been added/removed or properties of any display have changed

    /**
        Get CPU temperature in celsius.
        \note Only supported on Android for now.
    */
    float32 GetCpuTemperature() const;

private:
    void UpdateDisplayConfig();

    bool HandleEvent(const Private::MainDispatcherEvent& e);
    void HandleDisplayConfigChanged(const Private::MainDispatcherEvent& e);
    void HandleGamepadMotion(const Private::MainDispatcherEvent& e);
    void HandleGamepadButton(const Private::MainDispatcherEvent& e);
    void HandleGamepadAdded(const Private::MainDispatcherEvent& e);
    void HandleGamepadRemoved(const Private::MainDispatcherEvent& e);

    void Update(float32 frameDelta);
    void OnEngineInited();

    Vector<DisplayInfo> displays;

    Keyboard* keyboard = nullptr;
    Mouse* mouse = nullptr;
    Gamepad* gamepad = nullptr;
    TouchScreen* touchScreen = nullptr;
    Vector<InputDevice*> inputDevices;

    std::unique_ptr<Private::DeviceManagerImpl> impl;

    friend class Private::EngineBackend;
    friend struct Private::DeviceManagerImpl;
};

inline const DisplayInfo& DeviceManager::GetPrimaryDisplay() const
{
    // DeviceManagerImpl always places primary display as first element
    return displays[0];
}

inline const Vector<DisplayInfo>& DeviceManager::GetDisplays() const
{
    return displays;
}

inline size_t DeviceManager::GetDisplayCount() const
{
    return displays.size();
}

} // namespace DAVA
