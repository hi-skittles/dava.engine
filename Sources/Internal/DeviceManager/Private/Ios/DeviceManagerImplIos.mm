#include "DeviceManager/Private/Ios/DeviceManagerImplIos.h"

#if defined(__DAVAENGINE_IPHONE__)

#include <sys/utsname.h>

#include "DeviceManager/DeviceManager.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"

#import <UIKit/UIScreen.h>
#import <UIKit/UIDevice.h>

namespace DAVA
{
namespace Private
{
enum eIosDpi
{
    IPHONE_3_IPAD_MINI = 163,
    IPHONE_4_5_6_7_8_SE_IPAD_MINI2_MINI3 = 326,
    IPAD_1_2 = 132,
    IPAD_3_4_AIR_AIR2_PRO = 264,
    IPHONE_6_PLUS_7_PLUS_8_PLUS = 401,
    IPHONE_X = 458,
    IPHONE_6_PLUS_ZOOM = 461,
};

static const CGFloat defaultDpi = 160;

float32 DeviceManagerImpl::GetIPhoneMainScreenDpi()
{
    struct AppleDevice
    {
        int minSide;
        int dpi;
        const char* machineTag;
    };

    static AppleDevice listOfAppleDevices[] =
    {
      { 320, IPHONE_3_IPAD_MINI, "" },
      { 640, IPHONE_4_5_6_7_8_SE_IPAD_MINI2_MINI3, "" },
      { 750, IPHONE_4_5_6_7_8_SE_IPAD_MINI2_MINI3, "" },
      { 768, IPAD_1_2, "" },
      { 768, IPHONE_3_IPAD_MINI, "mini" },
      { 1080, IPHONE_6_PLUS_7_PLUS_8_PLUS, "" },
      { 1125, IPHONE_X, "" },
      { 1242, IPHONE_6_PLUS_ZOOM, "" },
      { 1536, IPAD_3_4_AIR_AIR2_PRO, "" },
      { 1536, IPHONE_4_5_6_7_8_SE_IPAD_MINI2_MINI3, "mini" },
      { 1668, IPAD_3_4_AIR_AIR2_PRO, "" },
      { 2048, IPAD_3_4_AIR_AIR2_PRO, "" }
    };

    CGSize screenSize = [ ::UIScreen mainScreen].bounds.size;
    CGFloat screenScale = [ ::UIScreen mainScreen].scale;

    float32 dpi = static_cast<float32>(defaultDpi * screenScale);
    CGFloat minSide = std::min(screenSize.width * screenScale, screenSize.height * screenScale);

    // find possible device with calculated side
    List<AppleDevice*> possibleDevices;
    for (size_t i = 0, sz = std::extent<decltype(listOfAppleDevices)>(); i < sz; ++i)
    {
        if (listOfAppleDevices[i].minSide == minSide)
        {
            possibleDevices.push_back(&listOfAppleDevices[i]);
        }
    }

    // get device name
    struct utsname systemInfo;
    uname(&systemInfo);

    String thisMachine = systemInfo.machine;

    // search real device from possibles
    AppleDevice* realDevice = nullptr;
    for (auto d : possibleDevices)
    {
        if (thisMachine.find(d->machineTag) != String::npos)
        {
            realDevice = d;
        }
    }

    // if found - use real device dpi
    if (nullptr != realDevice)
    {
        dpi = realDevice->dpi;
    }

    return dpi;
}

DeviceManagerImpl::DeviceManagerImpl(DeviceManager* devManager, Private::MainDispatcher* dispatcher)
    : deviceManager(devManager)
    , mainDispatcher(dispatcher)
{
    UpdateDisplayConfig();
}

void DeviceManagerImpl::UpdateDisplayConfig()
{
    NSMutableArray<::UIScreen*>* screens = [[[ ::UIScreen screens] mutableCopy] autorelease];
    ::UIScreen* mainScreen = [ ::UIScreen mainScreen];

    // https://developer.apple.com/reference/uikit/uiscreen/1617812-screens?language=objc
    // Apple's documentation clearly says that [UIScreen screens] should contain at least one element - primary display
    // Despite of that iOS 8.0.2 returns empty array
    // We manually add mainScreen object to screens array to workaround this
    if ([screens count] == 0 && mainScreen != nil)
    {
        [screens addObject:mainScreen];
    }

    deviceManager->displays.clear();
    deviceManager->displays.reserve([screens count]);

    for (::UIScreen* screen in screens)
    {
        DisplayInfo displayInfo;

        CGRect screenRect = [screen bounds];
        CGFloat screenScale = [screen scale];

        if (screen == mainScreen)
        {
            if (screenRect.size.height > screenRect.size.width)
            {
                screenRect = CGRectMake(screenRect.origin.y, screenRect.origin.x, screenRect.size.height, screenRect.size.width);
            }

            float32 dpi = GetIPhoneMainScreenDpi();
            displayInfo.rawDpiX = dpi;
            displayInfo.rawDpiY = dpi;
            displayInfo.name = "mainScreen";
            displayInfo.primary = true;
        }
        else
        {
            DVASSERT(false, "DPI retriving isn't implemented");

            UIUserInterfaceIdiom idom = [[UIDevice currentDevice] userInterfaceIdiom];

            if (idom == UIUserInterfaceIdiomPad)
            {
                displayInfo.rawDpiX = IPAD_3_4_AIR_AIR2_PRO; // default pad dpi, can be changed
                displayInfo.rawDpiY = IPAD_3_4_AIR_AIR2_PRO; // default pad dpi, can be changed
                displayInfo.name = "padMainScreen";
            }
            else if (idom == UIUserInterfaceIdiomPhone)
            {
                displayInfo.rawDpiX = IPHONE_4_5_6_7_8_SE_IPAD_MINI2_MINI3; // default phone dpi, can be changed
                displayInfo.rawDpiY = IPHONE_4_5_6_7_8_SE_IPAD_MINI2_MINI3; // default phone dpi, can be changed
                displayInfo.name = "phoneMainScreen";
            }
            else
            {
                CGFloat defaultScreenDpi = defaultDpi * [ ::UIScreen mainScreen].scale;
                displayInfo.rawDpiX = defaultScreenDpi;
                displayInfo.rawDpiY = defaultScreenDpi;
                displayInfo.name = "unknownMainScreen";
            }
        }

        displayInfo.systemId = reinterpret_cast<uintptr_t>(screen);
        displayInfo.rect.x = screenRect.origin.x;
        displayInfo.rect.y = screenRect.origin.y;
        displayInfo.rect.dx = screenRect.size.width * screenScale;
        displayInfo.rect.dy = screenRect.size.height * screenScale;

        if ([[[UIDevice currentDevice] systemVersion] compare:@"10.3" options:NSNumericSearch] != NSOrderedAscending)
        {
            displayInfo.maxFps = [screen maximumFramesPerSecond];
        }

        deviceManager->displays.push_back(displayInfo);
    }
}

float32 DeviceManagerImpl::GetCpuTemperature() const
{
    return 0.0f;
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_IPHONE__
