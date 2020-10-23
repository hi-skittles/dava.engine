#pragma once

#include <Base/ScopedPtr.h>
#include <CommandLine/ProgramOptions.h>
#include <FileSystem/FilePath.h>
#include <Input/ActionSystem.h>
#include <Math/Math2D.h>

namespace DAVA
{
class Engine;
class Window;
namespace Net
{
struct IChannelListener;
class ServicesProvider;
}
}

class UIViewScreen;
class UIViewerApp final
{
public:
    UIViewerApp(DAVA::Engine& e, const DAVA::Vector<DAVA::String>& cmdLine);

    void OnAppStarted();
    void OnWindowCreated(DAVA::Window* w);
    void OnWindowDestroyed(DAVA::Window* w);
    void OnAppFinished();

    void OnSuspend();
    void OnResume();

    void BeginFrame();
    void Draw(DAVA::Window* window);
    void EndFrame();

private:
    void OnWindowSizeChanged(DAVA::Window* window, DAVA::Size2f size, DAVA::Size2f surfaceSize);
    void OnActionTriggered(DAVA::Action action);

    void CreateDocumentsFolder();

    UIViewScreen* uiViewScreen = nullptr;
    DAVA::Engine& engine;

    DAVA::Size2f physicalToVirtualScale;

    DAVA::ProgramOptions options;
    bool optionsAreParsed = true;

    std::unique_ptr<DAVA::Net::ServicesProvider> servicesProvider;
    std::shared_ptr<DAVA::Net::IChannelListener> netLogger;
};
