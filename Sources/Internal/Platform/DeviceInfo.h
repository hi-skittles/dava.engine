#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"
#include "Functional/Signal.h"
#include "Render/RenderBase.h"

namespace DAVA
{
class DeviceInfoPrivate;

class DeviceInfo
{
public:
    static const int32 SIGNAL_STRENGTH_UNKNOWN = -1;

    enum ePlatform
    {
        PLATFORM_MACOS = 0,
        PLATFORM_IOS,
        PLATFORM_IOS_SIMULATOR,
        PLATFORM_ANDROID,
        PLATFORM_WIN32,
        PLATFORM_DESKTOP_WIN_UAP,
        PLATFORM_PHONE_WIN_UAP,
        PLATFORM_LINUX,
        PLATFORM_UNKNOWN_VALUE,
        PLATFORMS_COUNT
    };

    enum eNetworkType
    {
        NETWORK_TYPE_NOT_CONNECTED = 0,
        NETWORK_TYPE_UNKNOWN,
        NETWORK_TYPE_CELLULAR,
        NETWORK_TYPE_WIFI,
        NETWORK_TYPE_WIMAX,
        NETWORK_TYPE_ETHERNET,
        NETWORK_TYPE_BLUETOOTH,
        NETWORK_TYPES_COUNT
    };

    struct NetworkInfo
    {
        eNetworkType networkType;
        int32 signalStrength; //(0-no signal, 100 - max signal)

        NetworkInfo()
        {
            networkType = NETWORK_TYPE_UNKNOWN;
            signalStrength = SIGNAL_STRENGTH_UNKNOWN;
        }
    };

    enum eStorageType
    {
        STORAGE_TYPE_UNKNOWN = -1,
        STORAGE_TYPE_INTERNAL = 0,
        STORAGE_TYPE_PRIMARY_EXTERNAL,
        STORAGE_TYPE_SECONDARY_EXTERNAL,

        STORAGE_TYPES_COUNT
    };

    struct StorageInfo
    {
        eStorageType type;

        int64 totalSpace;
        int64 freeSpace;

        bool readOnly;
        bool removable;
        bool emulated;

        FilePath path;

        StorageInfo()
            : type(STORAGE_TYPE_UNKNOWN)
            , totalSpace(0)
            , freeSpace(0)
            , readOnly(false)
            , removable(false)
            , emulated(false)
        {
        }
    };

    //human interface device(HID)
    enum eHIDType
    {
        HID_UNKNOWN_TYPE = -1,
        HID_POINTER_TYPE,
        HID_MOUSE_TYPE,
        HID_JOYSTICK_TYPE,
        HID_GAMEPAD_TYPE,
        HID_KEYBOARD_TYPE,
        HID_KEYPAD_TYPE,
        HID_SYSTEM_CONTROL_TYPE,
        HID_TOUCH_TYPE,
        HID_COUNT_TYPE,
    };
    using ListForStorageInfo = List<StorageInfo>;

    static ePlatform GetPlatform();
    static String GetPlatformString();
    static String GetVersion();
    static String GetManufacturer();
    static String GetModel();
    static String GetLocale();
    static String GetRegion();
    static String GetTimeZone();
    static String GetUDID();
    static WideString GetName();
    static String GetHTTPProxyHost();
    static String GetHTTPNonProxyHosts();
    static int32 GetHTTPProxyPort();
    static int32 GetZBufferSize();
    static eGPUFamily GetGPUFamily();
    static NetworkInfo GetNetworkInfo();
    static List<StorageInfo> GetStoragesList();
    static int32 GetCpuCount();
    static bool IsTouchPresented();
    static String GetCarrierName();

    // true if device connected
    static bool IsHIDConnected(eHIDType type);

    // Override real gpu family
    static void SetOverridenGPU(eGPUFamily newGPU);
    // Reset override
    static void ResetOverridenGPU();

    // Signal type telling HID connected/disconnected
    // DeviceInfo::eHIDType value - type of HID
    // bool value - device's state: connected (true) or disconnected (false)
    using HIDConnectionSignal = Signal<eHIDType, bool>;
    static HIDConnectionSignal& GetHIDConnectionSignal(eHIDType type);
    static Signal<const String&> carrierNameChanged;

private:
    static DeviceInfoPrivate* GetPrivateImpl();
};
}
