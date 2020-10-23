#include "ClipboardImpl.Macos.h"
#include "Utils/NSStringUtils.h"

#import <Foundation/Foundation.h>
#import <AppKit/NSPasteboard.h>

namespace DAVA
{
ClipboardImplMac::ClipboardImplMac()
{
}

ClipboardImplMac::~ClipboardImplMac()
{
}

bool ClipboardImplMac::IsReadyToUse() const
{
    return [NSPasteboard generalPasteboard] != nil;
}

bool ClipboardImplMac::Clear() const
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    if (pasteboard != nil)
    {
        [pasteboard clearContents];
        return true;
    }
    return false;
}

bool ClipboardImplMac::HasText() const
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    if (pasteboard != nil)
    {
        NSArray* classes = [[[NSArray alloc] initWithObjects:[NSString class], nil] autorelease];
        BOOL ok = [pasteboard canReadObjectForClasses:classes options:nil];
        return ok == YES;
    }
    return false;
}

bool ClipboardImplMac::SetText(const WideString& str)
{
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    if (pasteboard != nil)
    {
        NSString* stringToWrite = NSStringFromWideString(str);
        [pasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
        BOOL ok = [pasteboard setString:stringToWrite forType:NSStringPboardType];
        return ok == YES;
    }
    return false;
}

WideString ClipboardImplMac::GetText() const
{
    WideString outPut;
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    if (pasteboard != nil && HasText())
    {
        NSString* s = [pasteboard stringForType:NSStringPboardType];
        WideString wstr = WideStringFromNSString(s);
        return wstr;
    }
    return outPut;
}
}
