#include "FileSystem/FilePath.h"

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)

#import <Foundation/NSString.h>
#import <Foundation/NSBundle.h>

namespace DAVA
{

	
#if defined(__DAVAENGINE_IPHONE__)
void FilePath::InitializeBundleName()
{
    NSString* bundlePath = [[[NSBundle mainBundle] bundlePath] stringByAppendingString:@"/Data/"];
    SetBundleName([bundlePath UTF8String]);
}

#elif defined(__DAVAENGINE_MACOS__)
void FilePath::InitializeBundleName()
{
    
#if defined(DAVA_MACOS_DATA_PATH)
    NSString* dataPath = @DAVA_MACOS_DATA_PATH;
#else
    NSString* dataPath = @"/Contents/Resources/Data/";
#endif

    NSString* bundlePath = [[[NSBundle mainBundle] bundlePath] stringByAppendingString:dataPath];
    SetBundleName([bundlePath UTF8String]);
}
	
	#if defined(__DAVAENGINE_NPAPI__)
void FilePath::InitializeBundleNameNPAPI(const String& pathToNPAPIPlugin)
{
    NSString* pluginPath = [NSString stringWithCString:pathToNPAPIPlugin.c_str() encoding:NSASCIIStringEncoding];
    NSString* bundlePath = [pluginPath stringByAppendingString:@"/Contents/Resources/"];
    SetBundleName([bundlePath UTF8String]);
}
	#endif // #if defined (__DAVAENGINE_NPAPI__)

#endif //#elif defined(__DAVAENGINE_MACOS__)
}

#endif //#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)
