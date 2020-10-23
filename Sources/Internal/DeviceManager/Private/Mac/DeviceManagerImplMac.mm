#include "DeviceManager/Private/Mac/DeviceManagerImplMac.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#include "DeviceManager/DeviceManager.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"

#import <AppKit/NSScreen.h>
#import <AppKit/NSApplication.h>
#include <IOKit/graphics/IOGraphicsLib.h>

namespace DAVA
{
namespace Private
{
DeviceManagerImpl::DeviceManagerImpl(DeviceManager* devManager, Private::MainDispatcher* dispatcher)
    : deviceManager(devManager)
    , mainDispatcher(dispatcher)
{
    [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidChangeScreenParametersNotification
                                                      object:nil
                                                       queue:nil
                                                  usingBlock:^(NSNotification* note)
                                                             {
                                                               this->UpdateDisplayConfig();
                                                             }];
    UpdateDisplayConfig();
}

// Implemented below
NSString* ScreenNameForDisplay(NSNumber* screen_id);

void DeviceManagerImpl::UpdateDisplayConfig()
{
    const NSArray<NSScreen*>* screens = [NSScreen screens];
    deviceManager->displays.resize([screens count]);

    for (int i = 0; i < [screens count]; ++i)
    {
        NSScreen* screen = [screens objectAtIndex:i];
        NSDictionary<NSString*, id>* description = [screen deviceDescription];

        const NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];
        const CGSize displayPhysicalSize = CGDisplayScreenSize([[description objectForKey:@"NSScreenNumber"] unsignedIntValue]);
        const float32 dpiX = (displayPixelSize.width / displayPhysicalSize.width) * 25.4f;
        const float32 dpiY = (displayPixelSize.height / displayPhysicalSize.height) * 25.4f;

        NSNumber* displayId = [description objectForKey:@"NSScreenNumber"];

        const NSRect framePixels = [screen convertRectToBacking:[screen frame]];

        DisplayInfo displayInfo;
        displayInfo.systemId = [displayId unsignedIntValue];
        displayInfo.rect.x = framePixels.origin.x;
        displayInfo.rect.y = framePixels.origin.y;
        displayInfo.rect.dx = framePixels.size.width;
        displayInfo.rect.dy = framePixels.size.height;
        displayInfo.rawDpiX = dpiX;
        displayInfo.rawDpiY = dpiY;
        displayInfo.primary = (i == 0);
        displayInfo.name = String([ScreenNameForDisplay(displayId) UTF8String]);

        deviceManager->displays[i] = displayInfo;
    }
}

// The easier way to get Display name is to use CGDisplayIOServicePort, but it's deprecated
// So use a long way: http://stackoverflow.com/questions/20025868/cgdisplayioserviceport-is-deprecated-in-os-x-10-9-how-to-replace

// Returns the io_service_t (an int) corresponding to a CG display ID, or 0 on failure.
// The io_service_t should be released with IOObjectRelease when not needed.
io_service_t IOServicePortFromCGDisplayID(CGDirectDisplayID displayID)
{
    io_iterator_t iter;
    io_service_t serv, servicePort = 0;

    CFMutableDictionaryRef matching = IOServiceMatching("IODisplayConnect");

    // releases matching for us
    kern_return_t err = IOServiceGetMatchingServices(kIOMasterPortDefault, matching, &iter);
    if (err)
        return 0;

    while ((serv = IOIteratorNext(iter)) != 0)
    {
        CFDictionaryRef displayInfo;
        CFNumberRef vendorIDRef;
        CFNumberRef productIDRef;
        CFNumberRef serialNumberRef;

        displayInfo = IODisplayCreateInfoDictionary(serv, kIODisplayOnlyPreferredName);

        Boolean success;
        success = CFDictionaryGetValueIfPresent(displayInfo, CFSTR(kDisplayVendorID), (const void**)&vendorIDRef);
        success &= CFDictionaryGetValueIfPresent(displayInfo, CFSTR(kDisplayProductID), (const void**)&productIDRef);

        if (!success)
        {
            CFRelease(displayInfo);
            continue;
        }

        SInt32 vendorID;
        CFNumberGetValue(vendorIDRef, kCFNumberSInt32Type, &vendorID);
        SInt32 productID;
        CFNumberGetValue(productIDRef, kCFNumberSInt32Type, &productID);

        // If a serial number is found, use it.
        // Otherwise serial number will be nil (= 0) which will match with the output of 'CGDisplaySerialNumber'
        SInt32 serialNumber = 0;
        if (CFDictionaryGetValueIfPresent(displayInfo, CFSTR(kDisplaySerialNumber), (const void**)&serialNumberRef))
        {
            CFNumberGetValue(serialNumberRef, kCFNumberSInt32Type, &serialNumber);
        }

        // If the vendor and product id along with the serial don't match
        // then we are not looking at the correct monitor.
        // NOTE: The serial number is important in cases where two monitors
        //       are the exact same.
        if (CGDisplayVendorNumber(displayID) != vendorID ||
            CGDisplayModelNumber(displayID) != productID ||
            CGDisplaySerialNumber(displayID) != serialNumber)
        {
            CFRelease(displayInfo);
            continue;
        }

        servicePort = serv;
        CFRelease(displayInfo);
        break;
    }

    IOObjectRelease(iter);
    return servicePort;
}

// Get the name of the specified display
NSString* ScreenNameForDisplay(NSNumber* screen_id)
{
    CGDirectDisplayID displayID = [screen_id unsignedIntValue];

    io_service_t serv = IOServicePortFromCGDisplayID(displayID);
    if (serv == 0)
        return @"unknown";

    CFDictionaryRef info = IODisplayCreateInfoDictionary(serv, kIODisplayOnlyPreferredName);
    IOObjectRelease(serv);

    CFStringRef display_name;
    CFDictionaryRef names = (CFDictionaryRef)CFDictionaryGetValue(info, CFSTR(kDisplayProductName));

    if (!names ||
        !CFDictionaryGetValueIfPresent(names, CFSTR("en_US"), (const void**)&display_name))
    {
        // This may happen if a desktop Mac is running headless
        CFRelease(info);
        return @"unknown";
    }

    NSString* displayname = [NSString stringWithString:(__bridge NSString*)display_name];
    CFRelease(info);
    return displayname;
}

float32 DeviceManagerImpl::GetCpuTemperature() const
{
    return 0.0f;
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_MACOS__
