#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include <Iphlpapi.h>
#include <winsock2.h>
#include <collection.h>

#include "Logger/Logger.h"
#include "Debug/DVAssert.h"
#include "FileSystem/FileSystem.h"
#include "Utils/MD5.h"
#include "Utils/StringFormat.h"
#include "Utils/Utils.h"
#include "Utils/UTF8Utils.h"
#include "Base/GlobalEnum.h"

#include "Engine/Engine.h"
#include "Engine/Private/Win10/PlatformCoreWin10.h"
#include "Platform/TemplateWin32/DeviceInfo.Win10.h"

__DAVAENGINE_WIN_UAP_INCOMPLETE_IMPLEMENTATION__MARKER__
const wchar_t* KOSTIL_SURFACE_MOUSE = L"NTRG0001";
const wchar_t* KOSTIL_SURFACE_KEYBOARD = L"MSHW0029";

namespace DAVA
{
// MSDN:: https://msdn.microsoft.com/en-us/library/windows/hardware/ff541364(v=vs.85).aspx
const wchar_t* GUID_DEVINTERFACE_MOUSE = L"System.Devices.InterfaceClassGuid:=\"{378DE44C-56EF-11D1-BC8C-00A0C91405DD}\"";
const wchar_t* GUID_DEVINTERFACE_KEYBOARD = L"System.Devices.InterfaceClassGuid:=\"{884b96c3-56ef-11d1-bc8c-00a0c91405dd}\"";
const wchar_t* GUID_DEVINTERFACE_TOUCH = L"System.Devices.InterfaceClassGuid:=\"{4D1E55B2-F16F-11CF-88CB-001111000030}\"";
const char* DEFAULT_TOUCH_ID = "touchId";

DeviceInfoPrivate::DeviceInfoPrivate()
{
    using ::Windows::Foundation::Metadata::ApiInformation;
    using ::Windows::Devices::Input::TouchCapabilities;
    using ::Windows::System::Profile::AnalyticsInfo;
    using ::Windows::System::Profile::AnalyticsVersionInfo;
    using ::Windows::System::UserProfile::AdvertisingManager;
    using ::Windows::Security::ExchangeActiveSyncProvisioning::EasClientDeviceInformation;

    isMobileMode = Private::PlatformCore::IsPhoneContractPresent();
    platform = isMobileMode ? DeviceInfo::PLATFORM_PHONE_WIN_UAP : DeviceInfo::PLATFORM_DESKTOP_WIN_UAP;
    TouchCapabilities touchCapabilities;
    isTouchPresent = (1 == touchCapabilities.TouchPresent); //  Touch is always present in MSVS simulator
    if (isTouchPresent)
    {
        auto hidsAccessor(hids.GetAccessor());
        Set<String>& setIdDevices = (*(hidsAccessor))[TOUCH];
        setIdDevices.emplace(DEFAULT_TOUCH_ID);
    }

    AnalyticsVersionInfo ^ versionInfo = AnalyticsInfo::VersionInfo;
    Platform::String ^ deviceVersion = versionInfo->DeviceFamilyVersion;
    Platform::String ^ deviceFamily = versionInfo->DeviceFamily;
    String versionString = UTF8Utils::EncodeToUTF8(deviceVersion->Data());
    int64 versionInt = _atoi64(versionString.c_str());
    std::stringstream versionStream;
    versionStream << ((versionInt & 0xFFFF000000000000L) >> 48) << ".";
    versionStream << ((versionInt & 0x0000FFFF00000000L) >> 32) << ".";
    versionStream << ((versionInt & 0x00000000FFFF0000L) >> 16) << ".";
    versionStream << (versionInt & 0x000000000000FFFFL);
    version = versionStream.str();
    platformString = UTF8Utils::EncodeToUTF8(versionInfo->DeviceFamily->Data());

    // get device Manufacturer/ProductName/Name
    {
        EasClientDeviceInformation deviceInfo;

        // MSDN says SystemManufacturer can be empty, so we have to check it
        if (!deviceInfo.SystemManufacturer->IsEmpty())
        {
            manufacturer = UTF8Utils::EncodeToUTF8(deviceInfo.SystemManufacturer->Data());
        }

        // MSDN says SystemProductName can be empty, so we have to check it
        if (!deviceInfo.SystemProductName->IsEmpty())
        {
            // MSDN recommends to use deviceInfo.SystemSku and use deviceInfo.SystemProductName
            // only in case when SystemSku is empty.
            //
            // In good cases deviceInfo.SystemSku is something like "XIAOMITEST MI4", while
            // deviceInfo.SystemManufacturer = "XIAOMITEST" and deviceInfo.SystemProductName = "MI4".
            // But in real life this SystemSku field can contain some unpredictable information,
            // e.g. "To be filled by O.E.M." or "<FF><FF><FF>...".
            //
            // So we prefer to use deviceInfo.SystemProductName instead of recommended deviceInfo.SystemSku
            modelName = UTF8Utils::EncodeToUTF8(deviceInfo.SystemProductName->Data());
        }

        // MSDN says deviceInfo.FriendlyName shouldn't be empty
        deviceName = WideString(deviceInfo.FriendlyName->Data());
    }

    try
    {
        uDID = UTF8Utils::EncodeToUTF8(AdvertisingManager::AdvertisingId->Data());
    }
    catch (Platform::Exception ^ e)
    {
        Logger::Error("[DeviceInfo] failed to get AdvertisingId: hresult=0x%08X, message=%s", e->HResult, UTF8Utils::EncodeToUTF8(e->Message->Data()).c_str());
    }
    gpu = GPUFamily();
    if (isMobileMode)
    {
        InitCarrierLinesAsync();
    }

    if (isMobileMode)
    {
        // DeviceInfoPrivate constructor must be called in UI thread and DeviceInfo is explicitly instantiated in PlatformCore::OnLaunchedOrActivated
        // file Engine/Private/Win10/PlatformCoreWin10.cpp
        CheckContinuumMode();

        using namespace ::Windows::Foundation;
        using namespace ::Windows::UI::Core;
        // Windows does not provide triggers to detect whether device has entered or left Continuum mode.
        // Then connect to CoreWindow::SizeChanged event and check UserInteractionMode
        // http://stackoverflow.com/questions/34115039/how-do-i-detect-that-the-windows-mobile-transitioned-to-continuum-mode
        CoreWindow::GetForCurrentThread()->SizeChanged += ref new TypedEventHandler<CoreWindow ^, WindowSizeChangedEventArgs ^>([this](CoreWindow ^, WindowSizeChangedEventArgs ^ ) {
            CheckContinuumMode();
        });
    }
    CreateAndStartHIDWatcher();
}

void DeviceInfoPrivate::CheckContinuumMode()
{
    using namespace ::Windows::UI::ViewManagement;
    UserInteractionMode mode = UIViewSettings::GetForCurrentView()->UserInteractionMode;
    bool detectedContinuumMode = mode == UserInteractionMode::Mouse;
    if (detectedContinuumMode != isContinuumMode)
    {
        isContinuumMode = detectedContinuumMode;
        NotifyAllClients(TOUCH, !isContinuumMode);
    }
}

DeviceInfo::ePlatform DeviceInfoPrivate::GetPlatform()
{
    return platform;
}

String DeviceInfoPrivate::GetPlatformString()
{
    return platformString;
}

String DeviceInfoPrivate::GetVersion()
{
    return version;
}

String DeviceInfoPrivate::GetManufacturer()
{
    return manufacturer;
}

String DeviceInfoPrivate::GetModel()
{
    return modelName;
}

String DeviceInfoPrivate::GetLocale()
{
    using ::Windows::System::UserProfile::GlobalizationPreferences;
    return RTStringToString(GlobalizationPreferences::Languages->GetAt(0));
}

String DeviceInfoPrivate::GetRegion()
{
    using ::Windows::System::UserProfile::GlobalizationPreferences;
    return RTStringToString(GlobalizationPreferences::HomeGeographicRegion);
}

String DeviceInfoPrivate::GetTimeZone()
{
    using ::Windows::Globalization::Calendar;

    Calendar calendar;
    return RTStringToString(calendar.GetTimeZone());
}

String DeviceInfoPrivate::GetHTTPProxyHost()
{
    return "";
}

String DeviceInfoPrivate::GetHTTPNonProxyHosts()
{
    return "";
}

int DeviceInfoPrivate::GetHTTPProxyPort()
{
    return 0;
}

int DeviceInfoPrivate::GetZBufferSize()
{
    return zBufferSize;
}

String DeviceInfoPrivate::GetUDID()
{
    return uDID;
}

WideString DeviceInfoPrivate::GetName()
{
    return deviceName;
}

eGPUFamily DeviceInfoPrivate::GetGPUFamilyImpl()
{
    return gpu;
}

DeviceInfo::NetworkInfo DeviceInfoPrivate::GetNetworkInfo()
{
    using ::Windows::Networking::Connectivity::NetworkInformation;
    using ::Windows::Networking::Connectivity::ConnectionProfile;
    using ::Windows::Networking::Connectivity::NetworkAdapter;

    DeviceInfo::NetworkInfo networkInfo;
    ConnectionProfile ^ icp = NetworkInformation::GetInternetConnectionProfile();
    if (icp != nullptr)
    {
        NetworkAdapter ^ networkAdapter = nullptr;

        // Even though it's not documented, NetworkAdapter property getter can throw an exception
        try
        {
            networkAdapter = icp->NetworkAdapter;
        }
        catch (Platform::Exception ^ e)
        {
            Logger::Error("[DeviceInfo] failed to get NetworkAdapter: hresult=0x%08X, message=%s", e->HResult, UTF8Utils::EncodeToUTF8(e->Message->Data()).c_str());
        }

        if (networkAdapter != nullptr)
        {
            if (icp->IsWlanConnectionProfile)
            {
                networkInfo.networkType = DeviceInfo::NETWORK_TYPE_WIFI;
            }
            else if (icp->IsWwanConnectionProfile)
            {
                networkInfo.networkType = DeviceInfo::NETWORK_TYPE_CELLULAR;
            }
            else
            {
                // in other case Ethernet
                networkInfo.networkType = DeviceInfo::NETWORK_TYPE_ETHERNET;
            }
        }
    }

    return networkInfo;
}

bool FillStorageSpaceInfo(DeviceInfo::StorageInfo& storage_info)
{
    ULARGE_INTEGER freeBytesAvailable;
    ULARGE_INTEGER totalNumberOfBytes;
    ULARGE_INTEGER totalNumberOfFreeBytes;

    BOOL res = ::GetDiskFreeSpaceExA(storage_info.path.GetAbsolutePathname().c_str(),
                                     &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes);

    if (res == FALSE)
        return false;

    storage_info.totalSpace = totalNumberOfBytes.QuadPart;
    storage_info.freeSpace = freeBytesAvailable.QuadPart;

    return true;
}

List<DeviceInfo::StorageInfo> DeviceInfoPrivate::GetStoragesList()
{
    using ::Windows::Storage::KnownFolders;

    List<DeviceInfo::StorageInfo> result;
    FileSystem* fileSystem = FileSystem::Instance();

    //information about internal storage
    DeviceInfo::StorageInfo storage;
    storage.path = fileSystem->GetUserDocumentsPath();
    storage.type = DeviceInfo::STORAGE_TYPE_INTERNAL;
    if (FillStorageSpaceInfo(storage))
    {
        result.push_back(storage);
    }

    //information about removable storages
    storage.type = DeviceInfo::STORAGE_TYPE_PRIMARY_EXTERNAL;
    storage.removable = true;

    auto removableStorages = WaitAsync(KnownFolders::RemovableDevices->GetFoldersAsync());
    for (unsigned i = 0; i < removableStorages->Size; ++i)
    {
        Platform::String ^ path = removableStorages->GetAt(i)->Path;
        storage.path = UTF8Utils::EncodeToUTF8(path->Data());
        if (FillStorageSpaceInfo(storage))
        {
            result.push_back(storage);
            //all subsequent external storages are secondary
            storage.type = DeviceInfo::STORAGE_TYPE_SECONDARY_EXTERNAL;
        }
    }

    return result;
}

bool DeviceInfoPrivate::IsHIDConnected(DeviceInfo::eHIDType type)
{
    // continuum mode don't have touch
    if (DeviceInfo::HID_TOUCH_TYPE == type && isContinuumMode)
    {
        return false;
    }
    auto func = [type](HIDConvPair pair) -> bool {
        return pair.second == type;
    };
    auto it = std::find_if(HidConvSet.begin(), HidConvSet.end(), func);
    return IsEnabled(it->first);
}

bool DeviceInfoPrivate::IsTouchPresented()
{
    return isTouchPresent && !isContinuumMode; //  Touch is always present in MSVS simulator
}

String DeviceInfoPrivate::GetCarrierName()
{
    if (nullptr == carrierName)
    {
        return "";
    }
    return UTF8Utils::EncodeToUTF8(carrierName->Data());
}

void DeviceInfoPrivate::NotifyAllClients(NativeHIDType type, bool isConnected)
{
    auto func = [type](HIDConvPair pair) -> bool {
        return pair.first == type;
    };
    DeviceInfo::eHIDType hidType = std::find_if(HidConvSet.begin(), HidConvSet.end(), func)->second;

    DeviceInfo::HIDConnectionSignal* signal = &GetHIDConnectionSignal(hidType);
    RunOnMainThreadAsync([=] { signal->Emit(hidType, isConnected); });
}

eGPUFamily DeviceInfoPrivate::GPUFamily()
{
    return GPU_DX11;
}

void DeviceInfoPrivate::InitCarrierLinesAsync()
{
    auto asyncTask = ::concurrency::create_task([this]() {
        try
        {
            using ::Windows::ApplicationModel::Calls::PhoneLine;
            using ::Windows::ApplicationModel::Calls::PhoneCallManager;
            using ::Windows::ApplicationModel::Calls::PhoneLineWatcher;
            using ::Windows::ApplicationModel::Calls::PhoneLineWatcherEventArgs;
            using ::Windows::Foundation::TypedEventHandler;
            phoneCallStore = WaitAsync(PhoneCallManager::RequestStoreAsync());
            watcher = phoneCallStore->RequestLineWatcher();

            Platform::Guid defaultGuid = WaitAsync(phoneCallStore->GetDefaultLineAsync());
            PhoneLine ^ defaultLine = WaitAsync(PhoneLine::FromIdAsync(defaultGuid));
            // can't do it on main thread, main dispatcher not ready
            carrierName = defaultLine->NetworkName;

            auto lineAdded = ref new TypedEventHandler<PhoneLineWatcher ^, PhoneLineWatcherEventArgs ^>([this](PhoneLineWatcher ^, PhoneLineWatcherEventArgs ^ args) {
                OnCarrierLineAdded(args);
            });
            auto completed = ref new TypedEventHandler<PhoneLineWatcher ^, Platform::Object ^>([this](PhoneLineWatcher ^, Platform::Object ^ ) {
                watcher = nullptr;
            });

            watcher->LineAdded += lineAdded;
            watcher->EnumerationCompleted += completed;
            watcher->Start();
        }
        catch (Platform::COMException ^ e)
        {
            String str = UTF8Utils::EncodeToUTF8(e->Message->Data());
            Logger::Error("Error msg = %s, added <uap:Capability Name=\"phoneCall\" /> capabilities in Package.appxmanifest", str.c_str());
        }
    });
}

void DeviceInfoPrivate::OnCarrierLineAdded(::Windows::ApplicationModel::Calls::PhoneLineWatcherEventArgs ^ args)
{
    using ::Windows::ApplicationModel::Calls::PhoneLine;
    using ::Windows::ApplicationModel::Calls::PhoneLineTransport;
    using ::Windows::Foundation::TypedEventHandler;

    PhoneLine ^ line = WaitAsync(PhoneLine::FromIdAsync(args->LineId));
    if ((nullptr != line) && (line->Transport == PhoneLineTransport::Cellular))
    {
        phoneLines.insert(std::make_pair(line->Id, line));
        auto lineChange = ref new TypedEventHandler<PhoneLine ^, Platform::Object ^>([this](PhoneLine ^ line, Platform::Object ^ ) {
            OnCarrierLineChange(line);
        });
        line->LineChanged += lineChange;
    }
}

void DeviceInfoPrivate::OnCarrierLineChange(::Windows::ApplicationModel::Calls::PhoneLine ^ line)
{
    using ::Windows::ApplicationModel::Calls::PhoneCallStore;
    using ::Windows::ApplicationModel::Calls::PhoneCallManager;
    Platform::Guid guid = WaitAsync(phoneCallStore->GetDefaultLineAsync());
    if (guid == line->Id)
    {
        RunOnMainThreadAsync([=] {
            // must run on main thread
            if ((nullptr != line->NetworkName) && (line->NetworkName != carrierName))
            {
                carrierName = line->NetworkName;
                DeviceInfo::carrierNameChanged.Emit(UTF8Utils::EncodeToUTF8(carrierName->Data()));
            }
        });
    }
}

::Windows::Devices::Enumeration::DeviceWatcher ^ DeviceInfoPrivate::CreateDeviceWatcher(NativeHIDType type)
{
    using ::Windows::Foundation::TypedEventHandler;
    using ::Windows::Devices::Enumeration::DeviceWatcher;
    using ::Windows::Devices::Enumeration::DeviceInformation;
    using ::Windows::Devices::Enumeration::DeviceInformationUpdate;
    using ::Windows::Devices::HumanInterfaceDevice::HidDevice;

    hids.GetAccessor()->emplace(type, Set<String>());
    DeviceWatcher ^ watcher = nullptr;
    Platform::Collections::Vector<Platform::String ^> ^ requestedProperties = ref new Platform::Collections::Vector<Platform::String ^>();
    requestedProperties->Append("System.Devices.InterfaceClassGuid");
    requestedProperties->Append("System.ItemNameDisplay");
    if (MOUSE == type)
    {
        watcher = DeviceInformation::CreateWatcher(ref new Platform::String(GUID_DEVINTERFACE_MOUSE), requestedProperties);
    }
    else if (KEYBOARD == type)
    {
        watcher = DeviceInformation::CreateWatcher(ref new Platform::String(GUID_DEVINTERFACE_KEYBOARD), requestedProperties);
    }
    else if (TOUCH == type)
    {
        watcher = DeviceInformation::CreateWatcher(ref new Platform::String(GUID_DEVINTERFACE_TOUCH), requestedProperties);
    }
    else
    {
        watcher = DeviceInformation::CreateWatcher(HidDevice::GetDeviceSelector(USAGE_PAGE, type));
    }
    auto added = ref new TypedEventHandler<DeviceWatcher ^, DeviceInformation ^>([this, type](DeviceWatcher ^ watcher, DeviceInformation ^ information) {
        OnDeviceAdded(type, information);
    });
    auto removed = ref new TypedEventHandler<DeviceWatcher ^, DeviceInformationUpdate ^>([this, type](DeviceWatcher ^ watcher, DeviceInformationUpdate ^ information) {
        OnDeviceRemoved(type, information);
    });
    auto updated = ref new TypedEventHandler<DeviceWatcher ^, DeviceInformationUpdate ^>([this, type](DeviceWatcher ^ watcher, DeviceInformationUpdate ^ information) {
        OnDeviceUpdated(type, information);
    });
    if (TOUCH != type)
    {
        watcher->Added += added;
        watcher->Removed += removed;
    }
    watcher->Updated += updated;
    watcher->Start();
    return watcher;
}

void DeviceInfoPrivate::CreateAndStartHIDWatcher()
{
    watchers.emplace_back(CreateDeviceWatcher(POINTER));
    watchers.emplace_back(CreateDeviceWatcher(MOUSE));
    watchers.emplace_back(CreateDeviceWatcher(JOYSTICK));
    watchers.emplace_back(CreateDeviceWatcher(GAMEPAD));
    watchers.emplace_back(CreateDeviceWatcher(KEYBOARD));
    watchers.emplace_back(CreateDeviceWatcher(KEYPAD));
    watchers.emplace_back(CreateDeviceWatcher(SYSTEM_CONTROL));
    watchers.emplace_back(CreateDeviceWatcher(TOUCH));
}

void DeviceInfoPrivate::OnDeviceAdded(NativeHIDType type, ::Windows::Devices::Enumeration::DeviceInformation ^ information)
{
    if (!information->IsEnabled)
    {
        return;
    }
    //TODO: delete it, kostil for surface mouse
    if (MOUSE == type)
    {
        if (wcsstr(information->Id->Data(), KOSTIL_SURFACE_MOUSE) != nullptr)
        {
            return;
        }
    }
    //TODO: delete it, kostil for surface keyboard
    if (KEYBOARD == type)
    {
        if (wcsstr(information->Id->Data(), KOSTIL_SURFACE_KEYBOARD) != nullptr)
        {
            return;
        }
    }
    auto hidsAccessor(hids.GetAccessor());
    String id = RTStringToString(information->Id);
    Set<String>& setIdDevices = (*(hidsAccessor))[type];
    auto idIter = setIdDevices.find(id);
    if (idIter == setIdDevices.end())
    {
        setIdDevices.emplace(std::move(id));
        NotifyAllClients(type, true);
    }
}

void DeviceInfoPrivate::OnDeviceRemoved(NativeHIDType type, ::Windows::Devices::Enumeration::DeviceInformationUpdate ^ information)
{
    String id = RTStringToString(information->Id);
    auto hidsAccessor(hids.GetAccessor());
    Set<String>& setIdDevices = (*(hidsAccessor))[type];
    auto idIter = setIdDevices.find(id);
    if (idIter != setIdDevices.end())
    {
        setIdDevices.erase(idIter);
        NotifyAllClients(type, false);
    }
}

void DeviceInfoPrivate::OnDeviceUpdated(NativeHIDType type, ::Windows::Devices::Enumeration::DeviceInformationUpdate ^ information)
{
    using ::Windows::Devices::Input::TouchCapabilities;

    auto hidsAccessor(hids.GetAccessor());
    Set<String>& setIdDevices = (*(hidsAccessor))[type];
    if (TOUCH == type)
    {
        TouchCapabilities touchCapabilities;
        bool newState = (1 == touchCapabilities.TouchPresent);
        if (isTouchPresent != newState)
        {
            isTouchPresent = newState;
            if (isTouchPresent)
            {
                setIdDevices.emplace(DEFAULT_TOUCH_ID);
            }
            else
            {
                setIdDevices.erase(DEFAULT_TOUCH_ID);
            }
            NotifyAllClients(type, isTouchPresent);
        }
    }
    else
    {
        bool isEnabled = false;
        Windows::Foundation::Collections::IMapView<Platform::String ^, Platform::Object ^> ^ properties = information->Properties;
        if (properties->HasKey(L"System.Devices.InterfaceEnabled"))
        {
            try
            {
                isEnabled = safe_cast<bool>(properties->Lookup(L"System.Devices.InterfaceEnabled"));
            }
            catch (Platform::InvalidCastException ^ e)
            {
                Logger::FrameworkDebug("DeviceInfoPrivate::OnDeviceUpdated. Can't cast System.Devices.InterfaceEnabled.");
            }
            catch (Platform::OutOfBoundsException ^ e)
            {
                Logger::FrameworkDebug("DeviceInfoPrivate::OnDeviceUpdated. OutOfBoundsException.");
            }
        }
        String id = RTStringToString(information->Id);
        auto iterId = setIdDevices.find(id);
        if (isEnabled)
        {
            if (iterId == setIdDevices.end())
            {
                setIdDevices.emplace(std::move(id));
                NotifyAllClients(type, isEnabled);
            }
        }
        else
        {
            if (iterId != setIdDevices.end())
            {
                setIdDevices.erase(id);
                NotifyAllClients(type, isEnabled);
            }
        }
    }
}

bool DeviceInfoPrivate::IsEnabled(NativeHIDType type)
{
    auto hidsAccessor(hids.GetAccessor());
    Set<String>& setIdDevices = (*(hidsAccessor))[type];
    return (setIdDevices.size() > 0);
}
}

#endif // __DAVAENGINE_WIN_UAP__
