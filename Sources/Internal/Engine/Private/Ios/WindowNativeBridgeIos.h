#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/EngineTypes.h"
#include "Engine/Private/EnginePrivateFwd.h"
#include "FileSystem/KeyedArchive.h"

@class UIWindow;
@class UIView;
@class NSSet;

@class RenderView;
@class RenderViewController;
@class NativeViewPool;
@class VisibleFrameObserver;
@class ObjectiveCInteropWindow;

namespace DAVA
{
namespace Private
{
// Bridge between C++ and Objective-C for iOS's WindowImpl class
// Responsibilities:
//  - holds neccesary Objective-C objects
//  - posts events to dispatcher
//
// iOS window unions several Objective-C classes (UIView subclass,
// UIViewController subclass, etc) and each of these classes
// receives some kind of system notfications or events. WindowNativeBridge
// combines all window-related logic and processes events from Objective-C classes.
// Objective-C classes only forward its notifications to WindowNativeBridge.
//
// WindowNativeBridge is a friend of iOS's WindowImpl
struct WindowNativeBridge final
{
    WindowNativeBridge(WindowImpl* windowImpl, const KeyedArchive* engineOptions);
    ~WindowNativeBridge();

    void* GetHandle() const;
    bool CreateWindow();

    void TriggerPlatformEvents();

    void ApplicationDidBecomeOrResignActive(bool becomeActive);
    void ApplicationDidEnterForegroundOrBackground(bool foreground);

    void AddUIView(UIView* uiview);
    void RemoveUIView(UIView* uiview);

    UIView* GetUIViewFromPool(const char8* className);
    void ReturnUIViewToPool(UIView* view);

    void SetSurfaceScale(const float32 scale);

    //////////////////////////////////////////////////////////////////////////
    // Notifications from RenderViewController
    void LoadView();
    void OrientationChanged();
    void ViewWillTransitionToSize(float32 w, float32 h);

    //////////////////////////////////////////////////////////////////////////
    // Notifications from RenderView
    void TouchesBegan(NSSet* touches);
    void TouchesMoved(NSSet* touches);
    void TouchesEnded(NSSet* touches);

    //////////////////////////////////////////////////////////////////////////

    void PostSafeAreaInsetsChanged();

    WindowImpl* windowImpl = nullptr;
    Window* window = nullptr;
    MainDispatcher* mainDispatcher = nullptr;

    UIWindow* uiwindow = nullptr;
    RenderView* renderView = nullptr;
    RenderViewController* renderViewController = nullptr;
    NativeViewPool* nativeViewPool = nullptr;
    VisibleFrameObserver* visibleFrameObserver = nullptr;
    ObjectiveCInteropWindow* objcInterop = nullptr;
    float32 dpi = 0.f;

    const KeyedArchive* engineOptions = nullptr;
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_IPHONE__
