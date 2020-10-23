#include "Engine/Private/Android/WindowImplAndroid.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Engine/Window.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"
#include "Engine/Private/Android/AndroidBridge.h"
#include "Engine/Private/Android/PlatformCoreAndroid.h"

#include "Logger/Logger.h"
#include "Time/SystemTimer.h"

#include <android/input.h>
#include <android/keycodes.h>

extern "C"
{

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewOnResume(JNIEnv* env, jclass jclazz, jlong windowImplPointer)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->OnResume();
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewOnPause(JNIEnv* env, jclass jclazz, jlong windowImplPointer)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->OnPause();
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewOnSurfaceCreated(JNIEnv* env, jclass jclazz, jlong windowImplPointer, jobject jsurfaceView)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->SurfaceCreated(env, jsurfaceView);
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewOnSurfaceChanged(JNIEnv* env, jclass jclazz, jlong windowImplPointer, jobject surface, jint width, jint height, jint surfaceWidth, jint surfaceHeight, jint dpi)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->SurfaceChanged(env, surface, width, height, surfaceWidth, surfaceHeight, dpi);
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewOnSurfaceDestroyed(JNIEnv* env, jclass jclazz, jlong windowImplPointer)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->SurfaceDestroyed();
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewProcessEvents(JNIEnv* env, jclass jclazz, jlong windowImplPointer)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->ProcessProperties();
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewOnMouseEvent(JNIEnv* env, jclass jclazz, jlong windowImplPointer, jint action, jint buttonState, jfloat x, jfloat y, jfloat deltaX, jfloat deltaY, jint modifierKeys)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->OnMouseEvent(action, buttonState, x, y, deltaX, deltaY, modifierKeys);
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewOnTouchEvent(JNIEnv* env, jclass jclazz, jlong windowImplPointer, jint action, jint touchId, jfloat x, jfloat y, jint modifierKeys)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->OnTouchEvent(action, touchId, x, y, modifierKeys);
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewOnKeyEvent(JNIEnv* env, jclass jclazz, jlong windowImplPointer, jint action, jint keyScancode, jint keyVirtual, jint unicodeChar, jint modifierKeys, jboolean isRepeated)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->OnKeyEvent(action, keyScancode, keyVirtual, unicodeChar, modifierKeys, isRepeated == JNI_TRUE);
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewOnGamepadButton(JNIEnv* env, jclass jclazz, jlong windowImplPointer, jint deviceId, jint action, jint keyCode)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->OnGamepadButton(deviceId, action, keyCode);
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewOnGamepadMotion(JNIEnv* env, jclass jclazz, jlong windowImplPointer, jint deviceId, jint axis, jfloat value)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->OnGamepadMotion(deviceId, axis, value);
}

JNIEXPORT void JNICALL Java_com_dava_engine_DavaSurfaceView_nativeSurfaceViewOnVisibleFrameChanged(JNIEnv* env, jclass jclazz, jlong windowImplPointer, jint x, jint y, jint w, jint h)
{
    using DAVA::Private::WindowImpl;
    WindowImpl* wimpl = reinterpret_cast<WindowImpl*>(static_cast<uintptr_t>(windowImplPointer));
    wimpl->OnVisibleFrameChanged(x, y, w, h);
}

} // extern "C"

namespace DAVA
{
namespace Private
{
WindowImpl::WindowImpl(EngineBackend* engineBackend, Window* window)
    : engineBackend(engineBackend)
    , window(window)
    , mainDispatcher(engineBackend->GetDispatcher())
    , uiDispatcher(MakeFunction(this, &WindowImpl::UIEventHandler), MakeFunction(this, &WindowImpl::TriggerPlatformEvents))
{
}

WindowImpl::~WindowImpl()
{
    DVASSERT(surfaceView == nullptr);
}

void WindowImpl::Resize(float32 /*width*/, float32 /*height*/)
{
    // Android windows are always stretched to display size
}

void WindowImpl::Activate()
{
    // not supported on android
}

void WindowImpl::SetFullscreen(eFullscreen /*newMode*/)
{
    // Fullscreen mode cannot be changed on Android
}

void WindowImpl::Close(bool appIsTerminating)
{
    if (appIsTerminating)
    {
        // If application is terminating then free window resources on C++ side and send event
        // as if window has been destroyed. Engine ensures that Close with appIsTerminating with
        // true value is always called on termination.
        if (surfaceView != nullptr)
        {
            mainDispatcher->SendEvent(MainDispatcherEvent::CreateWindowDestroyedEvent(window), MainDispatcher::eSendPolicy::IMMEDIATE_EXECUTION);

            JNIEnv* env = JNI::GetEnv();
            env->DeleteGlobalRef(surfaceView);
            surfaceView = nullptr;
        }
    }
    else if (window->IsPrimary())
    {
        // Primary android window cannot be closed, instead quit application according to Engine rules.
        // TODO: later add ability to close secondary windows.
        engineBackend->Quit(0);
    }
}

void WindowImpl::SetTitle(const String& title)
{
    // Android window does not have title
}

void WindowImpl::SetMinimumSize(Size2f /*size*/)
{
    // Minimum size does not apply to android
}

void WindowImpl::RunAsyncOnUIThread(const Function<void()>& task)
{
    uiDispatcher.PostEvent(UIDispatcherEvent::CreateFunctorEvent(task));
}

void WindowImpl::RunAndWaitOnUIThread(const Function<void()>& task)
{
    uiDispatcher.SendEvent(UIDispatcherEvent::CreateFunctorEvent(task));
}

bool WindowImpl::IsWindowReadyForRender() const
{
    return GetHandle() != nullptr;
}

void WindowImpl::TriggerPlatformEvents()
{
    if (uiDispatcher.HasEvents())
    {
        try
        {
            triggerPlatformEvents(surfaceView);
        }
        catch (const JNI::Exception& e)
        {
            Logger::Error("WindowImpl::TriggerPlatformEvents failed: %s", e.what());
            DVASSERT(false, e.what());
        }
    }
}

void WindowImpl::SetSurfaceScaleAsync(const float32 scale)
{
    DVASSERT(scale > 0.0f && scale <= 1.0f);

    uiDispatcher.PostEvent(UIDispatcherEvent::CreateSetSurfaceScaleEvent(scale));
}

void WindowImpl::DoSetSurfaceScale(const float32 scale)
{
    surfaceScale = scale;

    const float32 surfaceWidth = windowWidth * scale;
    const float32 surfaceHeight = windowHeight * scale;
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowSizeChangedEvent(window, windowWidth, windowHeight, surfaceWidth, surfaceHeight, surfaceScale, dpi, eFullscreen::On));
}

jobject WindowImpl::CreateNativeControl(const char8* controlClassName, void* backendPointer)
{
    jobject object = nullptr;

    try
    {
        JNI::LocalRef<jstring> className = JNI::CStrToJavaString(controlClassName);
        object = createNativeControl(surfaceView, className, reinterpret_cast<jlong>(backendPointer));
    }
    catch (const JNI::Exception& e)
    {
        Logger::Error("[WindowImpl::CreateNativeControl] failed to create native control %s: %s", controlClassName, e.what());
    }

    return object;
}

void WindowImpl::SetCursorCapture(eCursorCapture mode)
{
    // not implemented
}

void WindowImpl::SetCursorVisibility(bool visible)
{
    // not implemented
}

void WindowImpl::UIEventHandler(const UIDispatcherEvent& e)
{
    switch (e.type)
    {
    case UIDispatcherEvent::FUNCTOR:
        e.functor();
        break;
    case UIDispatcherEvent::SET_SURFACE_SCALE:
        DoSetSurfaceScale(e.setSurfaceScaleEvent.scale);
        break;
    default:
        break;
    }
}

void WindowImpl::ReplaceAndroidNativeWindow(ANativeWindow* newAndroidWindow)
{
    if (androidWindow != nullptr)
    {
        ANativeWindow_release(androidWindow);
    }
    androidWindow = newAndroidWindow;
}

void WindowImpl::OnResume()
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, true));
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowFocusChangedEvent(window, true));
}

void WindowImpl::OnPause()
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowFocusChangedEvent(window, false));
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, false));
}

void WindowImpl::SurfaceCreated(JNIEnv* env, jobject surfaceViewInstance)
{
    // Here reference to java DavaSurfaceView instance is obtained
    if (surfaceView == nullptr)
    {
        surfaceView = env->NewGlobalRef(surfaceViewInstance);
    }
}

void WindowImpl::SurfaceChanged(JNIEnv* env, jobject surface, int32 width, int32 height, int32 surfaceWidth, int32 surfaceHeight, int32 displayDpi)
{
    {
        ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env, surface);

        mainDispatcher->PostEvent(MainDispatcherEvent::CreateFunctorEvent([this, nativeWindow]() {
            ReplaceAndroidNativeWindow(nativeWindow);
        }));
    }

    const float previousWindowWidth = windowWidth;
    const float previousWindowHeight = windowHeight;

    windowWidth = static_cast<float32>(width);
    windowHeight = static_cast<float32>(height);
    dpi = static_cast<float32>(displayDpi);

    if (firstTimeSurfaceChanged)
    {
        uiDispatcher.LinkToCurrentThread();

        try
        {
            surfaceViewJavaClass.reset(new JNI::JavaClass("com/dava/engine/DavaSurfaceView"));
            triggerPlatformEvents = surfaceViewJavaClass->GetMethod<void>("triggerPlatformEvents");
            createNativeControl = surfaceViewJavaClass->GetMethod<jobject, jstring, jlong>("createNativeControl");
        }
        catch (const JNI::Exception& e)
        {
            Logger::Error("[WindowImpl] failed to init java bridge: %s", e.what());
            DVASSERT(false, e.what());
        }

        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowCreatedEvent(window, windowWidth, windowHeight, surfaceWidth, surfaceHeight, dpi, eFullscreen::On));
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibilityChangedEvent(window, true));
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowFocusChangedEvent(window, true));

        firstTimeSurfaceChanged = false;
    }
    else
    {
        // If surface size has changed, post sizeChanged event
        // Otherwise we should reset renderer since surface has been recreated

        if (!FLOAT_EQUAL(previousWindowWidth, windowWidth) || !FLOAT_EQUAL(previousWindowHeight, windowHeight))
        {
            // Do not use passed surfaceWidth & surfaceHeight, instead calculate it based on current scale factor
            // To handle cases when a surface has been recreated with original size (e.g. when switched to another app and returned back)
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowSizeChangedEvent(window, windowWidth, windowHeight, windowWidth * surfaceScale, windowHeight * surfaceScale, surfaceScale, dpi, eFullscreen::On));
        }
        else
        {
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateFunctorEvent([this]() {
                engineBackend->ResetRenderer(this->window, !this->IsWindowReadyForRender());
            }));
        }

        mainDispatcher->PostEvent(MainDispatcherEvent::CreateFunctorEvent([this]() {
            if (engineBackend->IsSuspended())
            {
                engineBackend->DrawSingleFrameWhileSuspended();
            }
        }));
    }
}

void WindowImpl::SurfaceDestroyed()
{
    // Android documentation says that after surfaceDestroyed call is finished no one should touch the surface
    // So make a blocking call that resets native window pointer and renderer
    mainDispatcher->SendEvent(MainDispatcherEvent::CreateFunctorEvent([this]() {
        ReplaceAndroidNativeWindow(nullptr);
        engineBackend->ResetRenderer(this->window, true);
    }));
}

void WindowImpl::ProcessProperties()
{
    uiDispatcher.ProcessEvents();
}

void WindowImpl::OnMouseEvent(int32 action, int32 nativeButtonState, float32 x, float32 y, float32 deltaX, float32 deltaY, int32 nativeModifierKeys)
{
    eModifierKeys modifierKeys = GetModifierKeys(nativeModifierKeys);
    switch (action)
    {
    case AMOTION_EVENT_ACTION_MOVE:
    case AMOTION_EVENT_ACTION_HOVER_MOVE:
        if (lastMouseMoveX != x || lastMouseMoveY != y)
        {
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseMoveEvent(window, x, y, modifierKeys, false));
            lastMouseMoveX = x;
            lastMouseMoveY = y;
        }
        break;
    case AMOTION_EVENT_ACTION_SCROLL:
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowMouseWheelEvent(window, x, y, deltaX, deltaY, modifierKeys, false));
        break;
    default:
        break;
    }

    // What you should know about mouse handling on android:
    //  - android does not send which mouse button generates up event (ACTION_UP)
    //  - android sometimes does not send mouse button down events (ACTION_DOWN)
    //  - android sends mouse button states in every mouse event
    // So we manually track mouse button states and send appropriate events into dava.engine.
    std::bitset<MOUSE_BUTTON_COUNT> state = GetMouseButtonState(nativeButtonState);
    std::bitset<MOUSE_BUTTON_COUNT> change = mouseButtonState ^ state;
    if (change.any())
    {
        MainDispatcherEvent e = MainDispatcherEvent::CreateWindowMouseClickEvent(window, MainDispatcherEvent::MOUSE_BUTTON_UP, eMouseButtons::LEFT, x, y, 1, modifierKeys, false);
        for (size_t i = 0, n = change.size(); i < n; ++i)
        {
            if (change[i])
            {
                e.type = state[i] ? MainDispatcherEvent::MOUSE_BUTTON_DOWN : MainDispatcherEvent::MOUSE_BUTTON_UP;
                e.mouseEvent.button = static_cast<eMouseButtons>(i + 1);
                mainDispatcher->PostEvent(e);
            }
        }
    }
    mouseButtonState = state;
}

void WindowImpl::OnTouchEvent(int32 action, int32 touchId, float32 x, float32 y, int32 nativeModifierKeys)
{
    MainDispatcherEvent::eType type = MainDispatcherEvent::TOUCH_DOWN;
    switch (action)
    {
    case AMOTION_EVENT_ACTION_MOVE:
        type = MainDispatcherEvent::TOUCH_MOVE;
        break;
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP:
        type = MainDispatcherEvent::TOUCH_UP;
        break;
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
        type = MainDispatcherEvent::TOUCH_DOWN;
        break;
    default:
        return;
    }

    eModifierKeys modifierKeys = GetModifierKeys(nativeModifierKeys);
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowTouchEvent(window, type, touchId, x, y, modifierKeys));
}

void WindowImpl::OnKeyEvent(int32 action, int32 keyScancode, int32 keyVirtual, int32 unicodeChar, int32 nativeModifierKeys, bool isRepeated)
{
    if (keyVirtual == AKEYCODE_BACK)
    {
        // backButtonDown check handles the case when button was pressed in another activity
        // 1. some third-party activity is open
        // 2. third-party activity closes on back button down event
        // 3. our activity is activated and handles corresponding back button up event, which is undesired
        if (backButtonDown && (action == AKEY_EVENT_ACTION_UP))
        {
            mainDispatcher->PostEvent(MainDispatcherEvent(MainDispatcherEvent::BACK_NAVIGATION));
        }

        backButtonDown = action == AKEY_EVENT_ACTION_DOWN;
    }
    else
    {
        bool isPressed = action == AKEY_EVENT_ACTION_DOWN;
        eModifierKeys modifierKeys = GetModifierKeys(nativeModifierKeys);
        MainDispatcherEvent::eType type = isPressed ? MainDispatcherEvent::KEY_DOWN : MainDispatcherEvent::KEY_UP;
        mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(window, type, keyScancode, keyVirtual, modifierKeys, isRepeated));

        if (isPressed && unicodeChar != 0)
        {
            mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowKeyPressEvent(window, MainDispatcherEvent::KEY_CHAR, 0, unicodeChar, modifierKeys, isRepeated));
        }
    }
}

void WindowImpl::OnGamepadButton(int32 deviceId, int32 action, int32 keyCode)
{
    MainDispatcherEvent::eType type = action == AKEY_EVENT_ACTION_DOWN ? MainDispatcherEvent::GAMEPAD_BUTTON_DOWN : MainDispatcherEvent::GAMEPAD_BUTTON_UP;
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateGamepadButtonEvent(deviceId, type, keyCode));
}

void WindowImpl::OnGamepadMotion(int32 deviceId, int32 axis, float32 value)
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateGamepadMotionEvent(deviceId, axis, value));
}

void WindowImpl::OnVisibleFrameChanged(int32 x, int32 y, int32 width, int32 height)
{
    mainDispatcher->PostEvent(MainDispatcherEvent::CreateWindowVisibleFrameChangedEvent(window, x, y, width, height));
}

std::bitset<WindowImpl::MOUSE_BUTTON_COUNT> WindowImpl::GetMouseButtonState(int32 nativeButtonState)
{
    std::bitset<MOUSE_BUTTON_COUNT> state;
    state.set(0, (nativeButtonState & AMOTION_EVENT_BUTTON_PRIMARY) == AMOTION_EVENT_BUTTON_PRIMARY);
    state.set(1, (nativeButtonState & AMOTION_EVENT_BUTTON_SECONDARY) == AMOTION_EVENT_BUTTON_SECONDARY);
    state.set(2, (nativeButtonState & AMOTION_EVENT_BUTTON_TERTIARY) == AMOTION_EVENT_BUTTON_TERTIARY);

    // Map BUTTON_BACK & BUTTON_FORWARD to Ext1 and Ext2 buttons accordingly
    state.set(3, (nativeButtonState & AMOTION_EVENT_BUTTON_BACK) == AMOTION_EVENT_BUTTON_BACK);
    state.set(4, (nativeButtonState & AMOTION_EVENT_BUTTON_FORWARD) == AMOTION_EVENT_BUTTON_FORWARD);

    return state;
}

eModifierKeys WindowImpl::GetModifierKeys(int32 nativeModifierKeys)
{
    eModifierKeys result = eModifierKeys::NONE;
    if (nativeModifierKeys & AMETA_SHIFT_ON)
    {
        result |= eModifierKeys::SHIFT;
    }
    if (nativeModifierKeys & AMETA_ALT_ON)
    {
        result |= eModifierKeys::ALT;
    }
    if (nativeModifierKeys & AMETA_CTRL_ON)
    {
        result |= eModifierKeys::CONTROL;
    }
    return result;
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_ANDROID__
