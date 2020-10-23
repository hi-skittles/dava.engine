#include "Utils/Utils.h"

#ifdef __DAVAENGINE_IPHONE__
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#endif

#if defined(__DAVAENGINE_MACOS__)
#import <Foundation/NSThread.h>
#import <Foundation/NSURL.h>
#import <AppKit/AppKit.h>
#endif //#if defined(__DAVAENGINE_MACOS__)

namespace DAVA
{
	
#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_MACOS__)

WideString GetDeviceName()
{
#if defined(__DAVAENGINE_IPHONE__)
    NSString* string = [UIDevice currentDevice].name;
    WideString ws;
    int len = static_cast<int32>([string length]);
    ws.resize(len);
    for (int k = 0; k < len; ++k)
    {
        ws[k] = [string characterAtIndex:k];
    }
    return ws;
#else
    return L"MacOS";
#endif
}

bool IsDrawThread()
{
    return [NSThread isMainThread];
}

void OpenURL(const String& url)
{
    NSString* urlString = [NSString stringWithCString:url.c_str() encoding:NSASCIIStringEncoding];
    NSURL* urlToOpen = [NSURL URLWithString:urlString];
#if defined(__DAVAENGINE_IPHONE__)
    [[UIApplication sharedApplication] openURL:urlToOpen];
#else
    [[NSWorkspace sharedWorkspace] openURL:urlToOpen];
#endif
}

#endif

String GenerateGUID()
{
    NSUUID* UUID = [NSUUID UUID];
    return String([[UUID UUIDString] cStringUsingEncoding:NSNonLossyASCIIStringEncoding]);
}
	
#if defined(__DAVAENGINE_IPHONE__)
void DisableSleepTimer()
{
    UIApplication* app = [UIApplication sharedApplication];
    app.idleTimerDisabled = YES;
}

void EnableSleepTimer()
{
    UIApplication* app = [UIApplication sharedApplication];
    app.idleTimerDisabled = NO;
}

#endif

}; // end of namespace DAVA
