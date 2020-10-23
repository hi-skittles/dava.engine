#pragma once

#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include "Platform/DeviceInfoPrivateBase.h"
#include "Concurrency/ConcurrentObject.h"

namespace DAVA
{
class DeviceInfoPrivate : public DeviceInfoPrivateBase
{
public:
    DeviceInfoPrivate();
    DeviceInfo::ePlatform GetPlatform();
    String GetPlatformString();
    String GetVersion();
    String GetManufacturer();
    String GetModel();
    String GetLocale();
    String GetRegion();
    String GetTimeZone();
    String GetUDID();
    WideString GetName();
    String GetHTTPProxyHost();
    String GetHTTPNonProxyHosts();
    int GetHTTPProxyPort();
    int GetZBufferSize();
    eGPUFamily GetGPUFamilyImpl() override;
    DeviceInfo::NetworkInfo GetNetworkInfo();
    List<DeviceInfo::StorageInfo> GetStoragesList();
    bool IsHIDConnected(DeviceInfo::eHIDType type);
    bool IsTouchPresented();
    String GetCarrierName();

private:
    enum NativeHIDType
    {
        UNKNOWN = 0x00,
        POINTER = 0x01,
        MOUSE = 0x02,
        JOYSTICK = 0x04,
        GAMEPAD = 0x05,
        KEYBOARD = 0x06,
        KEYPAD = 0x07,
        SYSTEM_CONTROL = 0x80,
        TOUCH = 0xFF
    };
    const uint16 USAGE_PAGE = 0x01;
    using HIDConvPair = std::pair<NativeHIDType, DeviceInfo::eHIDType>;
    Vector<HIDConvPair> HidConvSet =
    {
      { UNKNOWN, DeviceInfo::HID_UNKNOWN_TYPE },
      { POINTER, DeviceInfo::HID_POINTER_TYPE },
      { MOUSE, DeviceInfo::HID_MOUSE_TYPE },
      { JOYSTICK, DeviceInfo::HID_JOYSTICK_TYPE },
      { GAMEPAD, DeviceInfo::HID_GAMEPAD_TYPE },
      { KEYBOARD, DeviceInfo::HID_KEYBOARD_TYPE },
      { KEYPAD, DeviceInfo::HID_KEYPAD_TYPE },
      { SYSTEM_CONTROL, DeviceInfo::HID_SYSTEM_CONTROL_TYPE },
      { TOUCH, DeviceInfo::HID_TOUCH_TYPE }
    };

    Windows::Devices::Enumeration::DeviceWatcher ^ CreateDeviceWatcher(NativeHIDType type);
    void CreateAndStartHIDWatcher();
    void OnDeviceAdded(NativeHIDType type, Windows::Devices::Enumeration::DeviceInformation ^ information);
    void OnDeviceRemoved(NativeHIDType type, Windows::Devices::Enumeration::DeviceInformationUpdate ^ information);
    void OnDeviceUpdated(NativeHIDType type, Windows::Devices::Enumeration::DeviceInformationUpdate ^ information);
    bool IsEnabled(NativeHIDType type);
    void NotifyAllClients(NativeHIDType type, bool isConnected);
    eGPUFamily GPUFamily();

    void CheckContinuumMode();

    bool isTouchPresent = false;
    bool isMousePresent = false;
    bool isKeyboardPresent = false;
    bool isMobileMode = false;
    bool isContinuumMode = false;
    bool watchersCreated = false;

    ConcurrentObject<Map<NativeHIDType, Set<String>>> hids;

    Vector<Windows::Devices::Enumeration::DeviceWatcher ^> watchers;

    DeviceInfo::ePlatform platform = DeviceInfo::PLATFORM_UNKNOWN_VALUE;
    eGPUFamily gpu = GPU_INVALID;
    String platformString;
    String version;
    String manufacturer;
    String modelName;
    String uDID;
    WideString deviceName;
    int32 zBufferSize = 24;

    void OnCarrierLineAdded(::Windows::ApplicationModel::Calls::PhoneLineWatcherEventArgs ^ args);
    void OnCarrierLineChange(::Windows::ApplicationModel::Calls::PhoneLine ^ line);
    void InitCarrierLinesAsync();
    ::Windows::ApplicationModel::Calls::PhoneCallStore ^ phoneCallStore;
    ::Windows::ApplicationModel::Calls::PhoneLineWatcher ^ watcher;
    Map<Platform::Guid, ::Windows::ApplicationModel::Calls::PhoneLine ^> phoneLines;
    Platform::String ^ carrierName = nullptr;
};
}

#endif //  (__DAVAENGINE_WIN_UAP__)
