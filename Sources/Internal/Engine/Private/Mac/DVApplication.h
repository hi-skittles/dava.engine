#pragma once

#if defined(__DAVAENGINE_MACOS__)

#import <AppKit/NSApplication.h>

@interface DVApplication : NSApplication

- (void)sendEvent:(NSEvent*)event;

@end

#endif