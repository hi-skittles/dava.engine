// Utility header with forward declarations of Engine-related public and private classes
namespace DAVA
{
class Engine;
class Window;
class EngineContext;

template <typename T>
class Dispatcher;

namespace PlatformApi
{
#if defined(__DAVAENGINE_WIN_UAP__)
namespace Win10
{
struct XamlApplicationListener;
}
#endif
}

namespace Private
{
struct MainDispatcherEvent;
struct UIDispatcherEvent;

using MainDispatcher = Dispatcher<MainDispatcherEvent>;
using UIDispatcher = Dispatcher<UIDispatcherEvent>;

class EngineBackend;
class PlatformCore;
class WindowImpl;

#if defined(__DAVAENGINE_QT__)

#elif defined(__DAVAENGINE_WIN32__)

#elif defined(__DAVAENGINE_WIN_UAP__)
ref struct WindowNativeBridge;
#elif defined(__DAVAENGINE_MACOS__)
struct CoreNativeBridge;
struct WindowNativeBridge;
#elif defined(__DAVAENGINE_IPHONE__)
struct CoreNativeBridge;
struct WindowNativeBridge;
#elif defined(__DAVAENGINE_ANDROID__)
struct AndroidBridge;
#elif defined(__DAVAENGINE_LINUX__)
#else
#error "Platform is not implemented"
#endif

} // namespace Private
} // namespace DAVA
