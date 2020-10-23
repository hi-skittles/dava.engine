#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_ANDROID__)

#include <bitset>

#include "Engine/EngineTypes.h"
#include "Engine/PlatformApiAndroid.h"
#include "Engine/Private/EnginePrivateFwd.h"
#include "Engine/Private/Dispatcher/UIDispatcher.h"
#include "Engine/EngineTypes.h"

#include <android/native_window_jni.h>

namespace rhi
{
struct InitParam;
}

namespace DAVA
{
namespace Private
{
class WindowImpl final
{
public:
    WindowImpl(EngineBackend* engineBackend, Window* window);
    ~WindowImpl();

    WindowImpl(const WindowImpl&) = delete;
    WindowImpl& operator=(const WindowImpl&) = delete;

    void Resize(float32 width, float32 height);
    void Activate();
    void Close(bool appIsTerminating);
    void SetTitle(const String& title);
    void SetMinimumSize(Size2f size);
    void SetFullscreen(eFullscreen newMode);

    void RunAsyncOnUIThread(const Function<void()>& task);
    void RunAndWaitOnUIThread(const Function<void()>& task);

    void* GetHandle() const;

    bool IsWindowReadyForRender() const;
    void InitCustomRenderParams(rhi::InitParam& params);

    void TriggerPlatformEvents();

    void SetSurfaceScaleAsync(const float32 scale);

    jobject CreateNativeControl(const char8* controlClassName, void* backendPointer);

    void SetCursorCapture(eCursorCapture mode);
    void SetCursorVisibility(bool visible);
    // These methods are public intentionally as they are accessed from
    // extern "C" functions which are invoked by java
    void OnResume();
    void OnPause();
    void SurfaceCreated(JNIEnv* env, jobject surfaceViewInstance);
    void SurfaceChanged(JNIEnv* env, jobject surface, int32 width, int32 height, int32 surfWidth, int32 surfHeight, int32 displayDpi);
    void SurfaceDestroyed();
    void ProcessProperties();
    void OnMouseEvent(int32 action, int32 nativeButtonState, float32 x, float32 y, float32 deltaX, float32 deltaY, int32 nativeModifierKeys);
    void OnTouchEvent(int32 action, int32 touchId, float32 x, float32 y, int32 nativeModifierKeys);
    void OnKeyEvent(int32 action, int32 keyScancode, int32 keyVirtual, int32 unicodeChar, int32 nativeModifierKeys, bool isRepeated);
    void OnGamepadButton(int32 deviceId, int32 action, int32 keyCode);
    void OnGamepadMotion(int32 deviceId, int32 axis, float32 value);
    void OnVisibleFrameChanged(int32 x, int32 y, int32 width, int32 height);

private:
    // Shortcut for eMouseButtons::COUNT
    static const size_t MOUSE_BUTTON_COUNT = static_cast<size_t>(eMouseButtons::COUNT);

    void DoSetSurfaceScale(const float32 scale);
    void UIEventHandler(const UIDispatcherEvent& e);
    void ReplaceAndroidNativeWindow(ANativeWindow* newAndroidWindow);

    static std::bitset<MOUSE_BUTTON_COUNT> GetMouseButtonState(int32 nativeButtonState);
    static eModifierKeys GetModifierKeys(int32 nativeModifierKeys);

    EngineBackend* engineBackend = nullptr;
    Window* window = nullptr; // Window frontend reference
    MainDispatcher* mainDispatcher = nullptr; // Dispatcher that dispatches events to DAVA main thread
    UIDispatcher uiDispatcher; // Dispatcher that dispatches events to window UI thread

    jobject surfaceView = nullptr;
    ANativeWindow* androidWindow = nullptr;

    float32 lastMouseMoveX = -1; // Remember last mouse move position to detect
    float32 lastMouseMoveY = -1; // spurious mouse move events
    std::bitset<MOUSE_BUTTON_COUNT> mouseButtonState;

    std::unique_ptr<JNI::JavaClass> surfaceViewJavaClass;
    Function<void(jobject)> triggerPlatformEvents;
    Function<jobject(jobject, jstring, jlong)> createNativeControl;

    float32 surfaceScale = 1.0f;
    float32 windowWidth = 0.0f;
    float32 windowHeight = 0.0f;
    float32 dpi = 120.f;

    bool firstTimeSurfaceChanged = true;

    bool backButtonDown = false;

    // Friends
    friend struct AndroidBridge;
};

inline void* WindowImpl::GetHandle() const
{
    return androidWindow;
}

inline void WindowImpl::InitCustomRenderParams(rhi::InitParam& /*params*/)
{
    // No custom render params
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_ANDROID__
