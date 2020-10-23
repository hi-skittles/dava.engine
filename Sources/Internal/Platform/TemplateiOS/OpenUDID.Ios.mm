#include "OpenUDID.Ios.h"

#import <UIKit/UIPasteboard.h>
#import <UIKit/UIKit.h>

@implementation OpenUDIDiOS

- (void)setDict:(id)dict forPasteboard:(id)pboard
{
    [pboard setData:[NSKeyedArchiver archivedDataWithRootObject:dict] forPasteboardType:kOpenUDIDDomain];
}

- (id)getDataForPasteboard:(id)pboard
{
    return [pboard dataForPasteboardType:kOpenUDIDDomain];
}

- (id)getPasteboardWithName:(NSString*)name shouldCreate:(BOOL)create setPersistent:(BOOL)persistent
{
    UIPasteboard* slotPB = [UIPasteboard pasteboardWithName:name create:create];
    if (persistent)
    {
        [slotPB setPersistent:YES];
    }

    return (id)slotPB;
}
@end
