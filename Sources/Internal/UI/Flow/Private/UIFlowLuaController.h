#pragma once

#include "FileSystem/FilePath.h"
#include "Reflection/Reflection.h"
#include "UI/Flow/UIFlowController.h"

namespace DAVA
{
class UIFlowContext;
class UIControl;
class LuaScript;

/**
    Specialization of UIFlowController which use Lua script as Flow controller.

    Lua script can contains next functions:
        - `init(context)`,
        - `activate(context, view)`,
        - `loadResources(context, view)`,
        - `process(frameDelta)`,
        - `processEvent(eventName)` (must return true for avoid sending current event next),
        - `deactivate(context, view)`,
        - `unloadResources(context, view)`,
        - `release(context)`.
*/
class UIFlowLuaController final : public UIFlowController
{
    DAVA_VIRTUAL_REFLECTION(UIFlowLuaController, UIFlowController);

public:
    /** Constructor with specified Lua script path. */
    UIFlowLuaController(const FilePath& scriptPath);
    ~UIFlowLuaController() override;

    void Init(UIFlowContext* context) override;
    void Release(UIFlowContext* context) override;
    void LoadResources(UIFlowContext* context, UIControl* view) override;
    void UnloadResources(UIFlowContext* context, UIControl* view) override;
    void Activate(UIFlowContext* context, UIControl* view) override;
    void Deactivate(UIFlowContext* context, UIControl* view) override;
    void Process(float32 elapsedTime) override;
    bool ProcessEvent(const FastName& eventName, const Vector<Any>& params = Vector<Any>()) override;

private:
    bool loaded = false;
    bool hasProcess = false;
    bool hasProcessEvent = false;
    std::unique_ptr<LuaScript> script;
};
}