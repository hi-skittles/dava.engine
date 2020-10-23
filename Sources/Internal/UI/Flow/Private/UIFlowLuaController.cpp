#include "UI/Flow/Private/UIFlowLuaController.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Scripting/LuaScript.h"
#include "UI/Flow/UIFlowContext.h"
#include "UI/UIControl.h"

namespace DAVA
{
static const String INIT_FNAME = "init";
static const String RELEASE_FNAME = "release";
static const String LOAD_FNAME = "loadResources";
static const String UNLOAD_FNAME = "unloadResources";
static const String ACTIVATE_FNAME = "activate";
static const String DEACTIVATE_FNAME = "deactivate";
static const String PROCESS_FNAME = "process";
static const String PROCESS_EVENT_FNAME = "processEvent";

DAVA_VIRTUAL_REFLECTION_IMPL(UIFlowLuaController)
{
    ReflectionRegistrator<UIFlowLuaController>::Begin()
    .ConstructorByPointer<const FilePath&>()
    .DestructorByPointer([](UIFlowLuaController* c) { delete c; })
    .End();
}

UIFlowLuaController::UIFlowLuaController(const FilePath& scriptPath)
    : script(std::make_unique<LuaScript>())
{
    try
    {
        script->ExecScript(scriptPath);
        loaded = true;
    }
    catch (Exception& e)
    {
        Logger::Error(e.what());
        return;
    }

    hasProcess = script->HasGlobalFunction(PROCESS_FNAME);
    hasProcessEvent = script->HasGlobalFunction(PROCESS_EVENT_FNAME);
}

UIFlowLuaController::~UIFlowLuaController() = default;

void UIFlowLuaController::Init(UIFlowContext* context)
{
    if (loaded && script->HasGlobalFunction(INIT_FNAME))
    {
        Reflection contextRef = Reflection::Create(ReflectedObject(context));
        script->ExecFunctionSafe(INIT_FNAME, contextRef);
    }
}

void UIFlowLuaController::Release(UIFlowContext* context)
{
    if (loaded && script->HasGlobalFunction(RELEASE_FNAME))
    {
        Reflection contextRef = Reflection::Create(ReflectedObject(context));
        script->ExecFunctionSafe(RELEASE_FNAME, contextRef);
    }
}

void UIFlowLuaController::LoadResources(UIFlowContext* context, UIControl* view)
{
    if (loaded && script->HasGlobalFunction(LOAD_FNAME))
    {
        Reflection contextRef = Reflection::Create(ReflectedObject(context));
        Reflection viewRef = Reflection::Create(ReflectedObject(view));
        script->ExecFunctionSafe(LOAD_FNAME, contextRef, viewRef);
    }
}

void UIFlowLuaController::UnloadResources(UIFlowContext* context, UIControl* view)
{
    if (loaded && script->HasGlobalFunction(UNLOAD_FNAME))
    {
        Reflection contextRef = Reflection::Create(ReflectedObject(context));
        Reflection viewRef = Reflection::Create(ReflectedObject(view));
        script->ExecFunctionSafe(UNLOAD_FNAME, contextRef, viewRef);
    }
}

void UIFlowLuaController::Activate(UIFlowContext* context, UIControl* view)
{
    if (loaded && script->HasGlobalFunction(ACTIVATE_FNAME))
    {
        Reflection contextRef = Reflection::Create(ReflectedObject(context));
        Reflection viewRef = Reflection::Create(ReflectedObject(view));
        script->ExecFunctionSafe(ACTIVATE_FNAME, contextRef, viewRef);
    }
}

void UIFlowLuaController::Deactivate(UIFlowContext* context, UIControl* view)
{
    if (loaded && script->HasGlobalFunction(DEACTIVATE_FNAME))
    {
        Reflection contextRef = Reflection::Create(ReflectedObject(context));
        Reflection viewRef = Reflection::Create(ReflectedObject(view));
        script->ExecFunctionSafe(DEACTIVATE_FNAME, contextRef, viewRef);
    }
}

void UIFlowLuaController::Process(float32 elapsedTime)
{
    if (hasProcess)
    {
        script->ExecFunctionSafe(PROCESS_FNAME, elapsedTime);
    }
}

bool UIFlowLuaController::ProcessEvent(const FastName& eventName, const Vector<Any>& params)
{
    if (hasProcessEvent)
    {
        // TODO: make fast without copy params
        Vector<Any> args;
        args.push_back(eventName);
        args.insert(args.end(), params.begin(), params.end());

        Vector<Any> results;
        if (script->ExecFunctionWithResultSafe(PROCESS_EVENT_FNAME, args, { Type::Instance<bool>() }, results))
        {
            DVASSERT(!results.empty(), "Lua function 'processEvent' should return boolean result");
            return results[0].Get<bool>();
        }
    }
    return false;
}
}
