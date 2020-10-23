#include "Base/Platform.h"
#include "Base/BaseTypes.h"
#include "Engine/Engine.h"
#include "Engine/Private/EngineBackend.h"
#include "Debug/DVAssert.h"

#if defined(__DAVAENGINE_QT__)
#include "Engine/PlatformApiQt.h"
#endif

#import <AppKit/NSAlert.h>
#import <Foundation/NSThread.h>

namespace DAVA
{
namespace Debug
{
int MessageBox(const String& title, const String& message, const Vector<String>& buttons, int defaultButton)
{
    using namespace DAVA::Private;

    DVASSERT(0 < buttons.size() && buttons.size() <= 3);
    DVASSERT(0 <= defaultButton && defaultButton < static_cast<int>(buttons.size()));

    int result = -1;
    auto showMessageBox = [&title, &message, &buttons, defaultButton, &result]()
    {
        if (!EngineBackend::showingModalMessageBox)
        {
            @autoreleasepool
            {
                NSAlert* alert = [[[NSAlert alloc] init] autorelease];
                [alert setMessageText:@(title.c_str())];
                [alert setInformativeText:@(message.c_str())];

                int i = 0;
                for (const String& s : buttons)
                {
                    NSButton* alertButton = [alert addButtonWithTitle:@(s.c_str())];
                    if (i == defaultButton)
                        [alertButton setKeyEquivalent:@"\r"];
                    else
                        [alertButton setKeyEquivalent:@""];
                    i += 1;
                }

                EngineBackend::showingModalMessageBox = true;
#if defined(__DAVAENGINE_QT__)
                Window* primaryWindow = GetPrimaryWindow();
                if (primaryWindow != nullptr && primaryWindow->IsAlive())
                    PlatformApi::Qt::AcquireWindowContext(primaryWindow);
#endif

                NSModalResponse response = [alert runModal];
                switch (response)
                {
                case NSAlertFirstButtonReturn:
                    result = 0;
                    break;
                case NSAlertSecondButtonReturn:
                    result = 1;
                    break;
                case NSAlertThirdButtonReturn:
                    result = 2;
                    break;
                default:
                    result = -1;
                    break;
                }
                
#if defined(__DAVAENGINE_QT__)
                if (primaryWindow != nullptr && primaryWindow->IsAlive())
                    PlatformApi::Qt::ReleaseWindowContext(primaryWindow);
#endif

                EngineBackend::showingModalMessageBox = false;
            }
        }
    };

    Window* primaryWindow = GetPrimaryWindow();
    if ([NSThread isMainThread])
    {
        showMessageBox();
    }
    else if (primaryWindow != nullptr && primaryWindow->IsAlive())
    {
        primaryWindow->RunOnUIThread(showMessageBox);
    }
    return result;
}

} // namespace Debug
} // namespace DAVA
