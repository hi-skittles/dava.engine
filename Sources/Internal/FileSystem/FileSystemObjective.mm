#include "FileSystem/FileSystem.h"

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)

#import <Foundation/NSString.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSPathUtilities.h>

namespace DAVA
{
const FilePath FileSystem::GetUserDocumentsPath()
{
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* bundlePath = [paths objectAtIndex:0];
    NSString* filePath = [bundlePath stringByAppendingString:@"/"];
    return FilePath(String([filePath cStringUsingEncoding:NSUTF8StringEncoding]));
}

const FilePath FileSystem::GetPublicDocumentsPath()
{
#if defined(__DAVAENGINE_MACOS__)
    return "/Users/Shared/";
#else // iOS
    // FIXME for now just use user documents path
    return GetUserDocumentsPath();
#endif
}

const FilePath FileSystem::GetHomePath()
{
    NSString* dirPath = NSHomeDirectory();
    return FilePath(String([[dirPath stringByAppendingString:@"/"] cStringUsingEncoding:NSUTF8StringEncoding]));
}
}

#endif //#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)
