#include "NSStringUtils.h"
#include "UTF8Utils.h"

#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)

namespace DAVA
{
// for example
// maxLength  6
// origStr    q123we
// newStr     q7777we
// replStr    7777
// remove 3, insert 4
// charsToInsert 6 - 6 + (6 + 4 - 7) = 3 //with cut
// return YES - if need apply changes, or NO
BOOL NSStringModified(const NSRange& origRange, const NSString* origStr, DAVA::int32 maxLength, NSString** replStr)
{
    BOOL stringModify = NO;
    NSUInteger replStrLength = [*replStr length];
    NSUInteger origStrLength = [origStr length];
    NSUInteger removeSymbols = origRange.length;
    NSUInteger insertSymbols = replStrLength;
    NSUInteger finalStrLength = replStrLength + origStrLength - removeSymbols;
    NSUInteger cutSymbols = 0;
    // only if need cut text
    if (maxLength > 0 && replStrLength > 0 && finalStrLength > maxLength)
    {
        NSUInteger charsToInsert = maxLength - origStrLength + removeSymbols;
        stringModify = YES;
        *replStr = DAVA::NSStringSafeCut(*replStr, charsToInsert);
        insertSymbols = [*replStr length];
        cutSymbols = replStrLength - insertSymbols;
    }
    return stringModify;
}

// replString input string
// newLength - sought length, may be less
// example replString = 1234(emoji > 2), newLength = 6
// outString = 1234
NSString* NSStringSafeCut(const NSString* replString, NSUInteger newLength)
{
    if (nullptr == replString || [replString length] == 0 || [replString length] <= newLength)
    {
        return @"";
    }
    NSUInteger charsToInsert = newLength;
    NSUInteger position = 0;
    NSRange rangeCharacter;
    NSInteger index = 0;
    do
    {
        rangeCharacter = [replString rangeOfComposedCharacterSequenceAtIndex:index];
        if ((rangeCharacter.location + rangeCharacter.length) > charsToInsert)
        {
            position = rangeCharacter.location;
            break;
        }
        position = rangeCharacter.location + rangeCharacter.length;
        index++;
    }
    while ((rangeCharacter.location + rangeCharacter.length) < charsToInsert);
    NSString* outString = [replString substringWithRange:NSMakeRange(0, position)];
    return outString;
}

NSString* NSStringFromString(const DAVA::String& str)
{
    NSStringEncoding encoding = CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingUTF8);
    NSString* nsstring = [[[NSString alloc] initWithBytes:str.c_str()
                                                   length:str.length()
                                                 encoding:encoding] autorelease];
    return nsstring;
}

NSString* NSStringFromWideString(const DAVA::WideString& str)
{
    NSStringEncoding encoding = CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingUTF32LE);
    const char* string = reinterpret_cast<const char*>(str.c_str());

    NSString* nsstring = [[[NSString alloc] initWithBytes:string
                                                   length:str.length() * sizeof(wchar_t)
                                                 encoding:encoding] autorelease];
    return nsstring;
}

String StringFromNSString(NSString* string)
{
    if (string)
    {
        const char* utf8Chars = [string UTF8String];
        if (utf8Chars)
        {
            return String(utf8Chars);
        }
    }

    return String();
}

WideString WideStringFromNSString(NSString* string)
{
    if (string)
    {
        NSStringEncoding encoding = CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingUTF32LE);
        NSData* data = [string dataUsingEncoding:encoding];

        const wchar_t* stringData = reinterpret_cast<const wchar_t*>(data.bytes);
        return WideString(stringData, data.length / sizeof(wchar_t));
    }
    else
    {
        return L"";
    }
}
}

#endif //#if defined (__DAVAENGINE_MACOS__) || defined (__DAVAENGINE_IPHONE__)
