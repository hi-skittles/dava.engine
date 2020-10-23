#include "OpenUDID.Macos.h"

#import <AppKit/NSPasteboard.h>

@implementation OpenUDIDMac

- (void)setDict:(id)dict forPasteboard:(id)pboard
{
    [pboard setData:[NSKeyedArchiver archivedDataWithRootObject:dict] forType:kOpenUDIDDomain];
}

- (id)getDataForPasteboard:(id)pboard
{
    return [pboard dataForType:kOpenUDIDDomain];
}

- (id)getPasteboardWithName:(NSString*)name shouldCreate:(BOOL)create setPersistent:(BOOL)persistent
{
    NSPasteboard* slotPB = [NSPasteboard pasteboardWithName:name];
    return reinterpret_cast<id>(slotPB);
}
@end
