#include "Tests/DeviceInfoTest.h"

#include "Infrastructure/TestBed.h"

using namespace DAVA;

DeviceInfoTest::DeviceInfoTest(TestBed& app)
    : BaseScreen(app, "DeviceInfoTest")
{
    DeviceInfo::GetHIDConnectionSignal(DAVA::DeviceInfo::eHIDType::HID_POINTER_TYPE).Connect(this, &DeviceInfoTest::OnInputChanged);
    DeviceInfo::GetHIDConnectionSignal(DAVA::DeviceInfo::eHIDType::HID_MOUSE_TYPE).Connect(this, &DeviceInfoTest::OnInputChanged);
    DeviceInfo::GetHIDConnectionSignal(DAVA::DeviceInfo::eHIDType::HID_JOYSTICK_TYPE).Connect(this, &DeviceInfoTest::OnInputChanged);
    DeviceInfo::GetHIDConnectionSignal(DAVA::DeviceInfo::eHIDType::HID_GAMEPAD_TYPE).Connect(this, &DeviceInfoTest::OnInputChanged);
    DeviceInfo::GetHIDConnectionSignal(DAVA::DeviceInfo::eHIDType::HID_KEYBOARD_TYPE).Connect(this, &DeviceInfoTest::OnInputChanged);
    DeviceInfo::GetHIDConnectionSignal(DAVA::DeviceInfo::eHIDType::HID_KEYPAD_TYPE).Connect(this, &DeviceInfoTest::OnInputChanged);
    DeviceInfo::GetHIDConnectionSignal(DAVA::DeviceInfo::eHIDType::HID_SYSTEM_CONTROL_TYPE).Connect(this, &DeviceInfoTest::OnInputChanged);
    DeviceInfo::GetHIDConnectionSignal(DAVA::DeviceInfo::eHIDType::HID_TOUCH_TYPE).Connect(this, &DeviceInfoTest::OnInputChanged);

    DeviceInfo::carrierNameChanged.Connect(this, &DeviceInfoTest::OnCarrierChanged);
}

void DeviceInfoTest::LoadResources()
{
    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    Size2i screenSize = GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();
    BaseScreen::LoadResources();
    info = new UIStaticText(Rect(0.f, 0.f, static_cast<float32>(screenSize.dx), static_cast<float32>(screenSize.dy)));
    info->SetTextColor(Color::White);
    info->SetFont(font);
    info->SetFontSize(20.f);
    info->SetMultiline(true);
    info->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    AddControl(info);
    UpdateTestInfo();

    //TODO: Initialize resources here
}

void DeviceInfoTest::UpdateTestInfo()
{
    if (nullptr == info)
    {
        return;
    }
    std::stringstream infoStream;

    infoStream << "DeviceInfo\n";
    infoStream << "GetPlatform() :" << int(DeviceInfo::GetPlatform()) << "\n";
    infoStream << "GetPlatformString() :" << DeviceInfo::GetPlatformString() << "\n";
    infoStream << "GetVersion() :" << DeviceInfo::GetVersion() << "\n";
    infoStream << "GetManufacturer() :" << DeviceInfo::GetManufacturer() << "\n";
    infoStream << "GetModel() :" << DeviceInfo::GetModel() << "\n";
    infoStream << "GetLocale() :" << DeviceInfo::GetLocale() << "\n";
    infoStream << "GetRegion() :" << DeviceInfo::GetRegion() << "\n";
    infoStream << "GetTimeZone() :" << DeviceInfo::GetTimeZone() << "\n";
    infoStream << "GetUDID() :" << DeviceInfo::GetUDID() << "\n";
    infoStream << "GetName() :" << UTF8Utils::EncodeToUTF8(DeviceInfo::GetName()) << "\n";
    infoStream << "GetHTTPProxyHost() :" << DeviceInfo::GetHTTPProxyHost() << "\n";
    infoStream << "GetHTTPNonProxyHosts() :" << DeviceInfo::GetHTTPNonProxyHosts() << "\n";
    infoStream << "GetHTTPProxyPort() :" << DeviceInfo::GetHTTPProxyPort() << "\n";
    infoStream << "GetZBufferSize() :" << DeviceInfo::GetZBufferSize() << "\n";
    infoStream << "GetGPUFamily() :" << int(DeviceInfo::GetGPUFamily()) << "\n";
    auto netInfo = DeviceInfo::GetNetworkInfo();
    infoStream << "GetNetworkInfo() :"
               << "networkType: ";
    switch (netInfo.networkType)
    {
    case DAVA::DeviceInfo::NETWORK_TYPE_WIFI:
        infoStream << "WiFi";
        break;
    case DAVA::DeviceInfo::NETWORK_TYPE_WIMAX:
        infoStream << "WiMAX";
        break;
    case DAVA::DeviceInfo::NETWORK_TYPE_CELLULAR:
        infoStream << "cellular";
        break;
    case DAVA::DeviceInfo::NETWORK_TYPE_ETHERNET:
        infoStream << "ethernet";
        break;
    case DAVA::DeviceInfo::NETWORK_TYPE_BLUETOOTH:
        infoStream << "bluetooth";
        break;
    case DAVA::DeviceInfo::NETWORK_TYPE_UNKNOWN:
        infoStream << "unknown";
        break;
    case DAVA::DeviceInfo::NETWORK_TYPE_NOT_CONNECTED:
        infoStream << "not connected";
        break;
    case DAVA::DeviceInfo::NETWORK_TYPES_COUNT: // just no warning
        break;
    };
    infoStream << ", signalStrength: " << int(netInfo.signalStrength) << "\n";
    infoStream << "GetStoragesList() :\n";
    auto list = DeviceInfo::GetStoragesList();
    for (auto item : list)
    {
        infoStream << "    type: " << item.type << " "
                   << "    freeSpace: " << item.freeSpace / (1024 * 1024 * 1024) << "Gb "
                   << "    totalSpace: " << item.totalSpace / (1024 * 1024 * 1024) << "Gb "
                   << "    emulated: " << item.emulated << " "
                   << "    readOnly: " << item.readOnly << " "
                   << "    removable: " << item.removable << " "
                   << "    path: " << item.path.GetAbsolutePathname() << "\n";
    }
    infoStream << "GetCpuCount() :" << std::dec << (DeviceInfo::GetCpuCount()) << "\n";
    infoStream << "GetCarrierName() :" << DeviceInfo::GetCarrierName() << "\n";

    infoStream << "HIDDevices() :";
    infoStream << "pointer(" << ((hidDevices[DeviceInfo::HID_POINTER_TYPE] = DeviceInfo::IsHIDConnected(DeviceInfo::HID_POINTER_TYPE)) ? "y" : "n") << "), ";
    infoStream << "mouse(" << ((hidDevices[DeviceInfo::HID_MOUSE_TYPE] = DeviceInfo::IsHIDConnected(DeviceInfo::HID_MOUSE_TYPE)) ? "y" : "n") << "), ";
    infoStream << "joystick(" << ((hidDevices[DeviceInfo::HID_JOYSTICK_TYPE] = DeviceInfo::IsHIDConnected(DeviceInfo::HID_JOYSTICK_TYPE)) ? "y" : "n") << "), ";
    infoStream << "gamepad(" << ((hidDevices[DeviceInfo::HID_GAMEPAD_TYPE] = DeviceInfo::IsHIDConnected(DeviceInfo::HID_GAMEPAD_TYPE)) ? "y" : "n") << "), ";
    infoStream << "keyboard(" << ((hidDevices[DeviceInfo::HID_KEYBOARD_TYPE] = DeviceInfo::IsHIDConnected(DeviceInfo::HID_KEYBOARD_TYPE)) ? "y" : "n") << "), ";
    infoStream << "keypad(" << ((hidDevices[DeviceInfo::HID_KEYPAD_TYPE] = DeviceInfo::IsHIDConnected(DeviceInfo::HID_KEYPAD_TYPE)) ? "y" : "n") << "), ";
    infoStream << "system(" << ((hidDevices[DeviceInfo::HID_SYSTEM_CONTROL_TYPE] = DeviceInfo::IsHIDConnected(DeviceInfo::HID_SYSTEM_CONTROL_TYPE)) ? "y" : "n") << "), ";
    infoStream << "touch(" << ((hidDevices[DeviceInfo::HID_TOUCH_TYPE] = DeviceInfo::IsHIDConnected(DeviceInfo::HID_TOUCH_TYPE)) ? "y" : "n") << ")";
    infoStream << "\n";

    info->SetUtf8Text(infoStream.str());
}

void DeviceInfoTest::OnInputChanged(DAVA::DeviceInfo::eHIDType hidType, bool connected)
{
    hidDevices[hidType] = connected;
    UpdateTestInfo();
}

void DeviceInfoTest::OnCarrierChanged(const DAVA::String&)
{
    UpdateTestInfo();
}

void DeviceInfoTest::UnloadResources()
{
    SafeRelease(info);

    BaseScreen::UnloadResources();
    //TODO: Release resources here
}
