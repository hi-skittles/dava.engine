#pragma once

#include "FileSystem/FilePath.h"
#include "Reflection/Reflection.h"
#include "UI/Script/UIScriptComponentController.h"

namespace DAVA
{
class UIContext;
class UIControl;
class LuaScript;

/**
    Lua Script component controller implementation.
    Implementation of UIScriptComponentController interface that based on Lua-code.

    Lua script can contains next functions:
        - `init(controlRef, componentRef)`,
        - `release(controlRef, componentRef)`.
        - `parametersChanged(controlRef, componentRef)`.
        - `process(controlRef, componentRef, frameDelta)`,
        - `processEvent(controlRef, componentRef, eventName,  ...)` (must return true for avoid sending current event next),
*/
class UILuaScriptComponentController : public UIScriptComponentController
{
    DAVA_VIRTUAL_REFLECTION(UILuaScriptComponentController, UIScriptComponentController);

public:
    /**
    Constructor from file.
     \param scriptPath Path to Lua-script file
    */
    UILuaScriptComponentController(const FilePath& scriptPath);
    ~UILuaScriptComponentController() override;

    void Init(UIScriptComponent* component) override;
    void Release(UIScriptComponent* component) override;
    void ParametersChanged(UIScriptComponent* component) override;
    void Process(UIScriptComponent* component, float32 elapsedTime) override;
    bool ProcessEvent(UIScriptComponent* component, const FastName& eventName, const Vector<Any>& params = Vector<Any>()) override;

private:
    bool loaded = false;
    bool hasProcess = false;
    bool hasProcessEvent = false;
    std::unique_ptr<LuaScript> script;
};
}