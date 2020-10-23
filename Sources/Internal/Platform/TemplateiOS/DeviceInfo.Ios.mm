#include "Base/BaseTypes.h"

#ifdef __DAVAENGINE_IPHONE__

#include "Platform/TemplateiOS/DeviceInfo.Ios.h"
#include "Utils/StringFormat.h"
#import "Utils/NSStringUtils.h"
#include "Base/GlobalEnum.h"
#include "Base/TemplateHelpers.h"
#include "Logger/Logger.h"

#import <UIKit/UIDevice.h>
#import <UIKit/UIKit.h>
#import <Foundation/NSLocale.h>

#include <sys/utsname.h>
#include <sys/statvfs.h>

#import <AdSupport/ASIdentifierManager.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <CoreTelephony/CTCarrier.h>

#import "Platform/Reachability.h"

namespace DAVA
{
static NSString* CarrierToCarrierName(CTCarrier* carrier)
{
    // Returns autorelease object

    NSString* result = [NSString string];
    if (carrier != nil)
    {
        NSString* carrierName = [carrier carrierName];
        if (carrierName != nil)
        {
            result = [[carrierName retain] autorelease];
        }
    }

    return result;
}

DeviceInfoPrivate::DeviceInfoPrivate()
{
    telephonyNetworkInfo = [[CTTelephonyNetworkInfo alloc] init];

    CTCarrier* phoneCarrier = [telephonyNetworkInfo subscriberCellularProvider];

    lastCarrierName = [CarrierToCarrierName(phoneCarrier) retain];

    telephonyNetworkInfo.subscriberCellularProviderDidUpdateNotifier =
    ^(CTCarrier* carrier)
    {
      NSString* newCarrierName = CarrierToCarrierName(carrier);
      if (![lastCarrierName isEqualToString:newCarrierName])
      {
          [lastCarrierName release];
          lastCarrierName = [newCarrierName retain];

          DeviceInfo::carrierNameChanged.Emit(StringFromNSString(lastCarrierName));
      }
    };
}

DeviceInfoPrivate::~DeviceInfoPrivate()
{
    [telephonyNetworkInfo release];
    [lastCarrierName release];
}

DeviceInfo::ePlatform DeviceInfoPrivate::GetPlatform()
{
	#if TARGET_IPHONE_SIMULATOR == 1
    return DeviceInfo::PLATFORM_IOS_SIMULATOR;
	#else
    return DeviceInfo::PLATFORM_IOS;
	#endif
}

String DeviceInfoPrivate::GetPlatformString()
{
    return GlobalEnumMap<DeviceInfo::ePlatform>::Instance()->ToString(GetPlatform());
}

String DeviceInfoPrivate::GetVersion()
{
    NSString* systemVersion = [[UIDevice currentDevice] systemVersion];
    return String([systemVersion UTF8String]);
}

String DeviceInfoPrivate::GetManufacturer()
{
    return "Apple inc.";
}

String DeviceInfoPrivate::GetModel()
{
    String model = "";

    if (GetPlatform() == DeviceInfo::PLATFORM_IOS_SIMULATOR)
    {
        model = [[[UIDevice currentDevice] model] UTF8String];
    }
    else
    {
        struct utsname systemInfo;
        uname(&systemInfo);

        NSString* modelName = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];

        //General
        if ([modelName hasPrefix:@"iPhone"])
            model = [modelName UTF8String];
        if ([modelName hasPrefix:@"iPad"])
            model = [modelName UTF8String];
        if ([modelName hasPrefix:@"iPod"])
            model = [modelName UTF8String];
        if ([modelName hasPrefix:@"AppleTV"])
            model = [modelName UTF8String];

        // iPhone
        if ([modelName hasPrefix:@"iPhone1,1"])
            model = "iPhone 1G";
        if ([modelName hasPrefix:@"iPhone1,2"])
            model = "iPhone 3G";
        if ([modelName hasPrefix:@"iPhone2,1"])
            model = "iPhone 3GS";
        if ([modelName hasPrefix:@"iPhone3,1"])
            model = "iPhone 4 GSM";
        if ([modelName hasPrefix:@"iPhone3,3"])
            model = "iPhone 4 CDMA";
        if ([modelName hasPrefix:@"iPhone4,1"])
            model = "iPhone 4S";
        if ([modelName hasPrefix:@"iPhone5,1"])
            model = "iPhone 5 GSM LTE";
        if ([modelName hasPrefix:@"iPhone5,2"])
            model = "iPhone 5 CDMA LTE";
        if ([modelName hasPrefix:@"iPhone5,3"])
            model = "iPhone 5C GSM";
        if ([modelName hasPrefix:@"iPhone5,4"])
            model = "iPhone 5C GSM+CDMA";
        if ([modelName hasPrefix:@"iPhone6,1"])
            model = "iPhone 5S GSM";
        if ([modelName hasPrefix:@"iPhone6,2"])
            model = "iPhone 5S GSM+CDMA";
        if ([modelName hasPrefix:@"iPhone7,1"])
            model = "iPhone 6 Plus";
        if ([modelName hasPrefix:@"iPhone7,2"])
            model = "iPhone 6";
        if ([modelName hasPrefix:@"iPhone8,1"])
            model = "iPhone 6s";
        if ([modelName hasPrefix:@"iPhone8,2"])
            model = "iPhone 6s Plus";
        if ([modelName hasPrefix:@"iPhone8,4"])
            model = "iPhone SE";
        if ([modelName hasPrefix:@"iPhone9,1"])
            model = "iPhone 7 CDMA";
        if ([modelName hasPrefix:@"iPhone9,3"])
            model = "iPhone 7 GSM";
        if ([modelName hasPrefix:@"iPhone9,2"])
            model = "iPhone 7 Plus CDMA";
        if ([modelName hasPrefix:@"iPhone9,4"])
            model = "iPhone 7 Plus GSM";
        if ([modelName hasPrefix:@"iPhone10,1"])
            model = "iPhone 8 CDMA";
        if ([modelName hasPrefix:@"iPhone10,4"])
            model = "iPhone 8 GSM";
        if ([modelName hasPrefix:@"iPhone10,2"])
            model = "iPhone 8 Plus CDMA";
        if ([modelName hasPrefix:@"iPhone10,5"])
            model = "iPhone 8 Plus GSM";
        if ([modelName hasPrefix:@"iPhone10,3"])
            model = "iPhone X CDMA";
        if ([modelName hasPrefix:@"iPhone10,6"])
            model = "iPhone X GSM";

        // iPad
        if ([modelName hasPrefix:@"iPad1,1"])
            model = "iPad 1";
        if ([modelName hasPrefix:@"iPad2,1"])
            model = "iPad 2 WiFi";
        if ([modelName hasPrefix:@"iPad2,2"])
            model = "iPad 2 3G GSM";
        if ([modelName hasPrefix:@"iPad2,3"])
            model = "iPad 2 3G CDMA";
        if ([modelName hasPrefix:@"iPad2,4"])
            model = "iPad 2 WiFi";
        if ([modelName hasPrefix:@"iPad2,5"])
            model = "iPad Mini WiFi";
        if ([modelName hasPrefix:@"iPad2,6"])
            model = "iPad Mini GSM LTE";
        if ([modelName hasPrefix:@"iPad2,7"])
            model = "iPad Mini GSM CDMA LTE";
        if ([modelName hasPrefix:@"iPad3,1"])
            model = "iPad 3 WiFi";
        if ([modelName hasPrefix:@"iPad3,2"])
            model = "iPad 3 CDMA LTE";
        if ([modelName hasPrefix:@"iPad3,3"])
            model = "iPad 3 GSM LTE";
        if ([modelName hasPrefix:@"iPad3,4"])
            model = "iPad 4 WiFi";
        if ([modelName hasPrefix:@"iPad3,5"])
            model = "iPad 4 GSM LTE";
        if ([modelName hasPrefix:@"iPad3,6"])
            model = "iPad 4 CDMA LTE";
        if ([modelName hasPrefix:@"iPad4,1"])
            model = "iPad 5 WiFi";
        if ([modelName hasPrefix:@"iPad4,2"])
            model = "iPad 5 GSM CDMA LTE";
        if ([modelName hasPrefix:@"iPad4,3"])
            model = "iPad 5 (China)";
        if ([modelName hasPrefix:@"iPad4,4"])
            model = "iPad Mini 2 WiFi";
        if ([modelName hasPrefix:@"iPad4,5"])
            model = "iPad Mini 2 GSM CDMA LTE";
        if ([modelName hasPrefix:@"iPad4,6"])
            model = "iPad Mini 2 (China)";
        if ([modelName hasPrefix:@"iPad4,7"])
            model = "iPad Mini 3 WiFi";
        if ([modelName hasPrefix:@"iPad4,8"])
            model = "iPad Mini 3 Cellular";
        if ([modelName hasPrefix:@"iPad4,9"])
            model = "iPad Mini 3 (China)";
        if ([modelName hasPrefix:@"iPad5,1"])
            model = "iPad Mini 4 WiFi";
        if ([modelName hasPrefix:@"iPad5,2"])
            model = "iPad Mini 4 Cellular";
        if ([modelName hasPrefix:@"iPad5,3"])
            model = "iPad 6 WiFi";
        if ([modelName hasPrefix:@"iPad5,4"])
            model = "iPad 6 Cellular";
        if ([modelName hasPrefix:@"iPad6,3"])
            model = "iPad Pro 9.7 WiFi";
        if ([modelName hasPrefix:@"iPad6,4"])
            model = "iPad Pro 9.7 Cellular";
        if ([modelName hasPrefix:@"iPad6,7"])
            model = "iPad Pro WiFi";
        if ([modelName hasPrefix:@"iPad6,8"])
            model = "iPad Pro Cellular";
        if ([modelName hasPrefix:@"iPad6,11"])
            model = "iPad 5th gen WiFi";
        if ([modelName hasPrefix:@"iPad6,12"])
            model = "iPad 5th gen Cellular";
        if ([modelName hasPrefix:@"iPad7,1"])
            model = "iPad Pro 12.9 2nd gen WiFi";
        if ([modelName hasPrefix:@"iPad7,2"])
            model = "iPad Pro 12.9 2nd gen Cellular";
        if ([modelName hasPrefix:@"iPad7,3"])
            model = "iPad Pro 10.5 WiFi";
        if ([modelName hasPrefix:@"iPad7,4"])
            model = "iPad Pro 10.5 Cellular";

        // iPod
        if ([modelName hasPrefix:@"iPod1,1"])
            model = "iPod Touch";
        if ([modelName hasPrefix:@"iPod2,1"])
            model = "iPod Touch 2G";
        if ([modelName hasPrefix:@"iPod3,1"])
            model = "iPod Touch 3G";
        if ([modelName hasPrefix:@"iPod4,1"])
            model = "iPod Touch 4G";
        if ([modelName hasPrefix:@"iPod5,1"])
            model = "iPod Touch 5G";

        //AppleTV
        if ([modelName hasPrefix:@"AppleTV1,1"])
            model = "AppleTV";
        if ([modelName hasPrefix:@"AppleTV2,1"])
            model = "AppleTV 2G";
        if ([modelName hasPrefix:@"AppleTV3,1"])
            model = "AppleTV 3G early 2012";
        if ([modelName hasPrefix:@"AppleTV3,2"])
            model = "AppleTV 3G early 2013";

        if (model.empty())
        {
            // Unknown at this moment, return what is returned by system.
            model = [modelName UTF8String];
        }
    }

    return model;
}

String DeviceInfoPrivate::GetLocale()
{
    NSLocale* english = [[[NSLocale alloc] initWithLocaleIdentifier:@"en_US"] autorelease];

    NSString* langID = [[NSLocale preferredLanguages] objectAtIndex:0];
    NSString* lang = [english displayNameForKey:NSLocaleLanguageCode value:langID];

    String res = Format("%s (%s)", [langID UTF8String], [lang UTF8String]);
    return res;
}

String DeviceInfoPrivate::GetRegion()
{
    NSLocale* english = [[[NSLocale alloc] initWithLocaleIdentifier:@"en_US"] autorelease];

    NSString* countryCode = [[NSLocale currentLocale] objectForKey:NSLocaleCountryCode];
    NSString* country = [english displayNameForKey:NSLocaleCountryCode value:countryCode];

    String res = Format("%s (%s)", [countryCode UTF8String], [country UTF8String]);
    return res;
}

String DeviceInfoPrivate::GetTimeZone()
{
    NSTimeZone* localTime = [NSTimeZone systemTimeZone];
    return [[localTime name] UTF8String];
}

String DeviceInfoPrivate::GetUDID()
{
    bool hasAdvertisingId = (NSClassFromString(@"ASIdentifierManager") != nil);

    bool iOSLowerThan7 = false;
    NSString* version = [NSString stringWithCString:GetVersion().c_str()
                                           encoding:[NSString defaultCStringEncoding]];
    if ([version compare:@"7.0" options:NSNumericSearch] == NSOrderedAscending)
    {
        iOSLowerThan7 = true;
    }

    NSString* udid = nil;
    if (iOSLowerThan7 || !hasAdvertisingId)
    {
        udid = @"";
    }
    else
    {
        udid = [[[ASIdentifierManager sharedManager] advertisingIdentifier] UUIDString];
    }

    return [udid UTF8String];
}

WideString DeviceInfoPrivate::GetName()
{
    NSString* deviceName = [[UIDevice currentDevice] name];

    NSStringEncoding pEncode = CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingUTF32LE);
    NSData* pSData = [deviceName dataUsingEncoding:pEncode];

    return WideString((wchar_t*)[pSData bytes], [pSData length] / sizeof(wchar_t));
}

// Not impletemted yet
String DeviceInfoPrivate::GetHTTPProxyHost()
{
    return "";
}

// Not impletemted yet
String DeviceInfoPrivate::GetHTTPNonProxyHosts()
{
    return "";
}

// Not impletemted yet
int32 DeviceInfoPrivate::GetHTTPProxyPort()
{
    return 0;
}

int32 DeviceInfoPrivate::GetZBufferSize()
{
    return 24;
}

eGPUFamily DeviceInfoPrivate::GetGPUFamilyImpl()
{
    return GPU_POWERVR_IOS;
}

DeviceInfo::NetworkInfo DeviceInfoPrivate::GetNetworkInfo()
{
    static const struct
    {
        NetworkStatus platformNetworkStatus;
        DeviceInfo::eNetworkType internalNetworkType;
    }
    networkStatusMap[] =
    {
      { NotReachable, DeviceInfo::NETWORK_TYPE_NOT_CONNECTED },
      { ReachableViaWiFi, DeviceInfo::NETWORK_TYPE_WIFI },
      { ReachableViaWWAN, DeviceInfo::NETWORK_TYPE_CELLULAR }
    };

    DeviceInfo::NetworkInfo networkInfo;

    Reachability* reachability = [Reachability reachabilityForInternetConnection];
    [reachability startNotifier];

    NetworkStatus networkStatus = [reachability currentReachabilityStatus];

    uint32 networkStatusMapCount = COUNT_OF(networkStatusMap);
    for (uint32 i = 0; i < networkStatusMapCount; i++)
    {
        if (networkStatusMap[i].platformNetworkStatus == networkStatus)
        {
            networkInfo.networkType = networkStatusMap[i].internalNetworkType;
            break;
        }
    }

    [reachability stopNotifier];

    // No way to determine signal strength under iOS.
    return networkInfo;
}

List<DeviceInfo::StorageInfo> DeviceInfoPrivate::GetStoragesList()
{
    List<DeviceInfo::StorageInfo> l;
    DeviceInfo::StorageInfo info;

    info.type = DeviceInfo::STORAGE_TYPE_INTERNAL;

    const char* home = ::getenv("HOME");
    if (nullptr == home)
    {
        Logger::Error("HOME env not found");
        return l;
    }

    struct statvfs stat_data;

    if (0 != ::statvfs(home, &stat_data))
    {
        Logger::Error("failed get filesystem info for path: %s, error: %s", home, ::strerror(errno));
        return l;
    }

    info.totalSpace = static_cast<int64>(stat_data.f_frsize) * stat_data.f_blocks;
    info.freeSpace = static_cast<int64>(stat_data.f_frsize) * stat_data.f_bavail;

    info.readOnly = false;
    info.removable = false;
    info.emulated = false;

    info.path = home;

    l.push_back(info);
    return l;
}

bool DeviceInfoPrivate::IsHIDConnected(DeviceInfo::eHIDType type)
{
    //TODO: remove this empty realization and implement detection of HID connection
    return DeviceInfo::eHIDType::HID_TOUCH_TYPE == type;
}

bool DeviceInfoPrivate::IsTouchPresented()
{
    //TODO: remove this empty realization and implement detection touch
    return true;
}

String DeviceInfoPrivate::GetCarrierName()
{
    return StringFromNSString(lastCarrierName);
}
}
#endif
