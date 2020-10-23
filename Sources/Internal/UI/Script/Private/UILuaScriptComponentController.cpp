#include "UI/Script/Private/UILuaScriptComponentController.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Scripting/LuaScript.h"
#include "UI/Script/UIScriptComponent.h"
#include "UI/UIControl.h"

namespace DAVA
{
namespace UILuaScriptComponentDetails
{
static const String INIT_FNAME = "init";
static const String RELEASE_FNAME = "release";
static const String CHANGED_FNAME = "parametersChanged";
static const String PROCESS_FNAME = "process";
static const String PROCESS_EVENT_FNAME = "processEvent";
}

DAVA_VIRTUAL_REFLECTION_IMPL(UILuaScriptComponentController)
{
    ReflectionRegistrator<UILuaScriptComponentController>::Begin()
    .ConstructorByPointer<const FilePath&>()
    .DestructorByPointer([](UILuaScriptComponentController* c) { delete c; })
    .End();
}

UILuaScriptComponentController::UILuaScriptComponentController(const FilePath& scriptPath)
    : script(std::make_unique<LuaScript>())
{
    try
    {
        script->ExecScript(scriptPath);
        loaded = true;

        hasProcess = script->HasGlobalFunction(UILuaScriptComponentDetails::PROCESS_FNAME);
        hasProcessEvent = script->HasGlobalFunction(UILuaScriptComponentDetails::PROCESS_EVENT_FNAME);
    }
    catch (Exception& e)
    {
        Logger::Warning(e.what());
    }
}

UILuaScriptComponentController::~UILuaScriptComponentController() = default;

void UILuaScriptComponentController::Init(UIScriptComponent* component)
{
    if (loaded && script->HasGlobalFunction(UILuaScriptComponentDetails::INIT_FNAME))
    {
        Reflection controlRef = Reflection::Create(ReflectedObject(component->GetControl()));
        Reflection componentRef = Reflection::Create(ReflectedObject(component));
        script->ExecFunctionSafe(UILuaScriptComponentDetails::INIT_FNAME, controlRef, componentRef);
    }
}

void UILuaScriptComponentController::Release(UIScriptComponent* component)
{
    if (loaded && script->HasGlobalFunction(UILuaScriptComponentDetails::RELEASE_FNAME))
    {
        Reflection controlRef = Reflection::Create(ReflectedObject(component->GetControl()));
        Reflection componentRef = Reflection::Create(ReflectedObject(component));
        script->ExecFunctionSafe(UILuaScriptComponentDetails::RELEASE_FNAME, controlRef, componentRef);
    }
}

void UILuaScriptComponentController::ParametersChanged(UIScriptComponent* component)
{
    if (loaded && script->HasGlobalFunction(UILuaScriptComponentDetails::CHANGED_FNAME))
    {
        Reflection controlRef = Reflection::Create(ReflectedObject(component->GetControl()));
        Reflection componentRef = Reflection::Create(ReflectedObject(component));
        script->ExecFunctionSafe(UILuaScriptComponentDetails::CHANGED_FNAME, controlRef, componentRef);
    }
}

void UILuaScriptComponentController::Process(UIScriptComponent* component, float32 elapsedTime)
{
    if (hasProcess)
    {
        Reflection controlRef = Reflection::Create(ReflectedObject(component->GetControl()));
        Reflection componentRef = Reflection::Create(ReflectedObject(component));
        script->ExecFunctionSafe(UILuaScriptComponentDetails::PROCESS_FNAME, controlRef, componentRef, elapsedTime);
    }
}

bool UILuaScriptComponentController::ProcessEvent(UIScriptComponent* component, const FastName& eventName, const Vector<Any>& params)
{
    if (hasProcessEvent)
    {
        Reflection controlRef = Reflection::Create(ReflectedObject(component->GetControl()));
        Reflection componentRef = Reflection::Create(ReflectedObject(component));
        Vector<Any> args;
        args.push_back(controlRef);
        args.push_back(componentRef);
        args.push_back(eventName);
        args.insert(args.end(), params.begin(), params.end());

        Vector<Any> results;
        if (script->ExecFunctionWithResultSafe(UILuaScriptComponentDetails::PROCESS_EVENT_FNAME, args, { Type::Instance<bool>() }, results))
        {
            DVASSERT(!results.empty(), "Lua function 'processEvent' should return boolean result");
            return results[0].Get<bool>();
        }
    }
    return false;
}
}
