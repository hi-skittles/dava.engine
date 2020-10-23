#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "Engine/EngineTypes.h"
#include "Functional/Function.h"
#include "Input/InputEvent.h"

namespace DAVA
{
class Engine;
class UIEvent;
namespace Private
{
class EngineBackend;
}

/**
    \defgroup input Input

    Input system is a part of the engine which is responsible for:
    - Managing input devices (keyboards, mouses, gamepads etc.)
    - Handling input events sent by a platform
    - Storing each input device's state
*/
class InputSystem final
{
    friend class Window;
    friend class Private::EngineBackend;

public:
    // Temporal method for backward compatibility
    // TODO: remove InputSystem::Instance() method
    static InputSystem* Instance();

    uint32 AddHandler(eInputDeviceTypes inputDeviceMask, const Function<bool(const InputEvent&)>& handler);
    void ChangeHandlerDeviceMask(uint32 token, eInputDeviceTypes newInputDeviceMask);
    void RemoveHandler(uint32 token);

    void DispatchInputEvent(const InputEvent& inputEvent);

private:
    InputSystem(Engine* engine);
    ~InputSystem();

    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;

    void EndFrame();
    void HandleInputEvent(UIEvent* uie); // TODO: remove this after finishing input system (it is used now for gestures and back button)

private:
    struct InputHandler
    {
        InputHandler(uint32 aToken, eInputDeviceTypes inputDeviceMask, const Function<bool(UIEvent*)>& handler);
        InputHandler(uint32 aToken, eInputDeviceTypes inputDeviceMask, const Function<bool(const InputEvent&)>& handler);

        uint32 token;
        bool useRawInputCallback;
        eInputDeviceTypes deviceMask;
        Function<bool(UIEvent*)> uiEventHandler;
        Function<bool(const InputEvent&)> rawInputHandler;
    };

    Vector<InputHandler> handlers;
    uint32 nextHandlerToken = 1;
    bool pendingHandlerRemoval = false;
};

inline InputSystem::InputHandler::InputHandler(uint32 aToken, eInputDeviceTypes inputDeviceMask, const Function<bool(UIEvent*)>& handler)
    : token(aToken)
    , useRawInputCallback(false)
    , deviceMask(inputDeviceMask)
    , uiEventHandler(handler)
{
}

inline InputSystem::InputHandler::InputHandler(uint32 aToken, eInputDeviceTypes inputDeviceMask, const Function<bool(const InputEvent&)>& handler)
    : token(aToken)
    , useRawInputCallback(true)
    , deviceMask(inputDeviceMask)
    , rawInputHandler(handler)
{
}

} // namespace DAVA
