#include "Engine/Engine.h"

#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"

namespace DAVA
{
namespace EngineSingletonNamespace
{
Engine* engineSingleton = nullptr;
}

Engine* Engine::Instance()
{
    return EngineSingletonNamespace::engineSingleton;
}

Engine::Engine()
{
    DVASSERT(EngineSingletonNamespace::engineSingleton == nullptr);

    EngineSingletonNamespace::engineSingleton = this;

    engineBackend = Private::EngineBackend::Instance();
    engineBackend->EngineCreated(this);
}

Engine::~Engine()
{
    engineBackend->EngineDestroyed();
    engineBackend = nullptr;

    EngineSingletonNamespace::engineSingleton = nullptr;
}

const EngineContext* Engine::GetContext() const
{
    return engineBackend->GetContext();
}

Window* Engine::PrimaryWindow() const
{
    return engineBackend->GetPrimaryWindow();
}

const Vector<Window*>& Engine::GetWindows() const
{
    return engineBackend->GetWindows();
}

eEngineRunMode Engine::GetRunMode() const
{
    return engineBackend->GetRunMode();
}

bool Engine::IsStandaloneGUIMode() const
{
    return engineBackend->IsStandaloneGUIMode();
}

bool Engine::IsEmbeddedGUIMode() const
{
    return engineBackend->IsEmbeddedGUIMode();
}

bool Engine::IsConsoleMode() const
{
    return engineBackend->IsConsoleMode();
}

void Engine::Init(eEngineRunMode runMode, const Vector<String>& modules, KeyedArchive* options)
{
    engineBackend->Init(runMode, modules, options);
}

int Engine::Run()
{
    return engineBackend->Run();
}

void Engine::QuitAsync(int exitCode)
{
    engineBackend->Quit(exitCode);
}

void Engine::Terminate(int exitCode)
{
    engineBackend->Terminate(exitCode);
}

void Engine::SetCloseRequestHandler(const Function<bool(Window*)>& handler)
{
    engineBackend->SetCloseRequestHandler(handler);
}

uint32 Engine::GetGlobalFrameIndex() const
{
    return engineBackend->GetGlobalFrameIndex();
}

const Vector<String>& Engine::GetCommandLine() const
{
    return engineBackend->GetCommandLine();
}

Vector<char*> Engine::GetCommandLineAsArgv() const
{
    return engineBackend->GetCommandLineAsArgv();
}

const KeyedArchive* Engine::GetOptions() const
{
    return engineBackend->GetOptions();
}

bool Engine::IsSuspended() const
{
    return engineBackend->IsSuspended();
}

void Engine::SetScreenTimeoutEnabled(bool enabled)
{
    engineBackend->SetScreenTimeoutEnabled(enabled);
}

Vector<String> Engine::GetStartupActivationFilenames() const
{
    return engineBackend->GetActivationFilenames();
}

} // namespace DAVA
